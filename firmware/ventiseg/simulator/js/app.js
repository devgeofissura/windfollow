/*
 * ventiseg — simulador local V0.1.
 * Espelha o contrato de docs/PROTOCOLO.md sem firmware, GPIO, rede ou placa.
 * O nucleo (VentisegCore) e puro e determinístico: recebe now_ms por Args.
 * A UI so e iniciada quando `document` existe; em Node so o nucleo e exposto.
 */
(function (global) {
  'use strict';

  // Constantes espelhadas do contrato — tests/test_simulator.py barra divergência.
  const SIM = Object.freeze({
    MAX_DATAGRAM_BYTES: 96,
    LINK_TIMEOUT_MS: 1200,
    INT32_MAX: 2147483647,
    DEAD_BAND: 55,
    MANUAL_STEP: 80,
    MAX_TRAVEL: 533,
    TRACK_GAIN: 0.75,
    CONTROL_PORT: 4211, // previsto no protocolo; firmware atual e serial-only
  });

  const STATES = Object.freeze(['SAFE', 'TRACK', 'MANUAL']);
  const FAULTS = Object.freeze(['None', 'Lnk', 'Range']);
  const X_LIMIT = 1000;

  // Tamanho em bytes UTF-8, sem depender de TextEncoder (Node + file://).
  function byteLength(text) {
    let bytes = 0;
    for (let i = 0; i < text.length; i += 1) {
      const code = text.codePointAt(i);
      if (code > 0xffff) i += 1; // par substituto conta como um codigo
      bytes += code <= 0x7f ? 1 : code <= 0x7ff ? 2 : code <= 0xffff ? 3 : 4;
    }
    return bytes;
  }

  function clamp(value, limit) {
    return Math.max(-limit, Math.min(limit, value));
  }

  // Mesma conta do laserseg: erro em x -> passos do eixo unico.
  function trackSteps(x, maxTravel, gain, deadBand) {
    if (!Number.isFinite(x)) return 0;
    if (Math.abs(x) <= deadBand) return 0;
    return clamp(Math.trunc(x * maxTravel * gain / 1000), maxTravel);
  }

  function createState() {
    return {
      mode: 'SAFE',          // SAFE | TRACK | MANUAL
      fault: 'None',         // None | Lnk | Range
      seq: -1,               // -1 ate a primeira amostra T aceita
      axis: 0,               // posicao comandada do eixo, em passos
      targetX: 0,            // ultimo x aceito
      valid: false,          // ultimo campo valid aceito
      moving: false,         // eixo com comando ativo
      lastAcceptedMs: null,  // so T aceito renova; null = nunca houve amostra
      linkLost: false,
    };
  }

  function telemetry(s) {
    return 'ACK,' + s.seq + ',' + s.mode + ',' + s.fault;
  }

  function linkExpired(s, nowMs) {
    // Antes da primeira amostra T o eixo ja esta SAFE e parado: sem fault Lnk.
    if (s.lastAcceptedMs === null) return false;
    return nowMs - s.lastAcceptedMs > SIM.LINK_TIMEOUT_MS;
  }

  let lastReason = '';
  let lastDirection = 0;

  // Motivo da ultima decisao, observavel sem alterar o estado da FSM.
  function lastReasonValue() {
    return lastReason;
  }

  // Decision isolada para testes diferenciais (nao muda a UI).
  function parseDecision(line, rxMs, nowMs, lastSeq) {
    const s = createState();
    if (lastSeq !== null && lastSeq !== undefined) s.seq = lastSeq;
    lastDirection = 0;
    const ok = feed(s, line,
      nowMs === null || nowMs === undefined ? rxMs : nowMs, rxMs);
    let kind = 'rejected';
    if (ok) {
      if (line.indexOf('TRACK,') === 0) kind = 'track';
      else if (line.indexOf('MANUAL,') === 0) kind = 'manual';
      else kind = 'sample';
    }
    return {
      kind,
      reason: ok ? '' : lastReason,
      accepted: ok,
      seq: s.seq,
      x: s.targetX,
      valid: kind === 'sample' ? s.valid : false,
      enabled: kind === 'track' ? s.mode === 'TRACK' : false,
      direction: kind === 'manual' ? lastDirection : 0,
    };
  }

  function fail(s, reason) {
    lastReason = reason;
    // Rejeicao por faixa e diagnostico, como em protocol_ref.py e parser.cpp.
    if (reason === 'range') s.fault = 'Range';
    return false;
  }

  function ageRejected(s, nowMs, rxMs) {
    const age = nowMs - rxMs;
    if (!Number.isFinite(age) || age < 0 || age > SIM.LINK_TIMEOUT_MS) {
      fail(s, 'age');
      return true;
    }
    return false;
  }

  /*
   * feed(): unica porta de entrada do protocolo.
   * Ordem: tamanho/codificacao -> formato -> faixa -> sequencia (so em T) ->
   * idade (qualquer comando, por ultimo). `rxMs` e separado de `nowMs` para
   * representar atraso de fila; quando omitido, rxMs = nowMs.
   * Retorna true se aceito. Comando rejeitado nao altera modo/eixo/seq/relogio;
   * apenas uma rejeicao por faixa reporta fault=Range.
   * Apenas T aceito renova o watchdog e limpa fault=Lnk (contrato v0.1).
   */
  function feed(s, line, nowMs, rxMs) {
    if (rxMs === undefined) rxMs = nowMs;
    if (nowMs === undefined) nowMs = rxMs;
    if (typeof line !== 'string') return fail(s, 'encoding');
    if (line.length === 0) return fail(s, 'length');
    if (byteLength(line) > SIM.MAX_DATAGRAM_BYTES) return fail(s, 'length');
    for (let i = 0; i < line.length; i += 1) {
      const code = line.codePointAt(i);
      if (code === 0 || code > 0x7f) return fail(s, 'encoding');
    }

    const track = /^TRACK,(-?[0-9]+)$/.exec(line);
    if (track) {
      const value = Number(track[1]);
      if (!Number.isSafeInteger(value) || (value !== 0 && value !== 1)) {
        return fail(s, 'range');
      }
      if (ageRejected(s, nowMs, rxMs)) return false;
      lastReason = '';
      s.mode = value === 1 ? 'TRACK' : 'SAFE';
      s.moving = s.mode === 'TRACK';
      if (!s.linkLost) s.fault = 'None';  // TRACK NAO renova watchdog
      return true;
    }

    const manual = /^MANUAL,(-?[0-9]+)$/.exec(line);
    if (manual) {
      const direction = Number(manual[1]);
      if (!Number.isSafeInteger(direction) ||
          (direction !== -1 && direction !== 0 && direction !== 1)) {
        return fail(s, 'range');
      }
      if (ageRejected(s, nowMs, rxMs)) return false;
      lastReason = '';
      lastDirection = direction;
      if (direction === 0) {
        s.mode = 'SAFE';
        s.moving = false;
      } else {
        s.mode = 'MANUAL';
        s.axis = clamp(s.axis + direction * SIM.MANUAL_STEP, SIM.MAX_TRAVEL);
        s.moving = true;
      }
      if (!s.linkLost) s.fault = 'None';  // MANUAL NAO renova watchdog
      return true;
    }

    const sample = /^T,(-?[0-9]+),(-?[0-9]+),(-?[0-9]+)$/.exec(line);
    if (sample) {
      const seq = Number(sample[1]);
      const x = Number(sample[2]);
      const valid = Number(sample[3]);
      if (!Number.isSafeInteger(seq) || !Number.isSafeInteger(x) ||
          !Number.isSafeInteger(valid)) {
        return fail(s, 'range');
      }
      if (seq < 0 || seq > SIM.INT32_MAX ||
          x < -X_LIMIT || x > X_LIMIT ||
          (valid !== 0 && valid !== 1)) {
        return fail(s, 'range');
      }
      if (seq <= s.seq) return fail(s, 'sequence');
      if (ageRejected(s, nowMs, rxMs)) return false;

      lastReason = '';
      s.seq = seq;
      s.targetX = x;
      s.valid = valid === 1;
      if (s.valid) {
        s.axis = trackSteps(x, SIM.MAX_TRAVEL, SIM.TRACK_GAIN, SIM.DEAD_BAND);
        s.moving = s.mode === 'TRACK';
      } else {
        s.axis = 0;
        s.moving = false;
      }
      s.fault = 'None';
      s.lastAcceptedMs = rxMs;  // watchdog conta do recebimento, nao da consulta
      s.linkLost = false;
      return true;
    }

    return fail(s, 'format');
  }

  // Watchdog: sem comando aceito em 1200 ms -> SAFE, eixo parado, fault=Lnk.
  function checkWatchdog(s, nowMs) {
    if (s.linkLost || !linkExpired(s, nowMs)) return false;
    s.mode = 'SAFE';
    s.moving = false;
    s.valid = false;
    s.fault = 'Lnk';
    s.linkLost = true;
    return true;
  }

  // Helper somente da simulacao: restaura o link sem mexer em modo/eixo.
  function relink(s, nowMs) {
    s.lastAcceptedMs = nowMs;
    s.linkLost = false;
    s.fault = 'None';
    return true;
  }

  // Injecao explicita de falha de faixa (simulacao; nao e comando de protocolo).
  function setSimFault(s, fault) {
    if (!FAULTS.includes(fault)) return false;
    s.fault = fault;
    return true;
  }

  const Core = Object.freeze({
    SIM, STATES, FAULTS, X_LIMIT,
    byteLength, clamp, trackSteps, createState, telemetry, linkExpired,
    feed, lastReason: lastReasonValue, parseDecision,
    checkWatchdog, relink, setSimFault,
  });

  global.VentisegCore = Core;

  // Em Node (testes) so o nucleo e carregado; nao ha DOM para ligar.
  if (typeof document === 'undefined') return;

  // ---------------------------------------------------------------- UI ----
  const $ = (id) => document.getElementById(id);
  const el = {
    theme: $('theme-toggle'), header: $('header-status'),
    tabs: Array.from(document.querySelectorAll('.tab')),
    panels: Array.from(document.querySelectorAll('.tab-panel')),
    stState: $('st-state'), stMotion: $('st-motion'), stAxis: $('st-axis'),
    stTarget: $('st-target'), stSeq: $('st-seq'), stLink: $('st-link'),
    stFault: $('st-fault'), stAck: $('st-ack'),
    caption: $('axis-caption'), marker: $('axis-marker'),
    btnTrack: $('btn-track'), btnLink: $('btn-link'), btnRange: $('btn-range'),
    btnReset: $('btn-reset'), form: $('inject-form'), cmd: $('cmd-line'),
    log: $('log'), constants: $('proto-constants'),
  };

  let sim = Core.createState();
  let nextSeq = 0;
  let syntheticTarget = 0;
  let linkCut = false;

  function clock() {
    return typeof performance !== 'undefined' && performance.now
      ? performance.now() : Date.now();
  }

  function addLog(kind, text) {
    const row = document.createElement('li');
    const time = document.createElement('span');
    time.className = 'time';
    time.textContent = new Date().toLocaleTimeString('pt-BR', { hour12: false });
    const dir = document.createElement('span');
    dir.className = 'dir ' + kind;
    dir.textContent = kind === 'tx' ? 'TX' : kind === 'rx' ? 'RX' : 'EV';
    const msg = document.createElement('span');
    msg.textContent = text;
    row.appendChild(time);
    row.appendChild(dir);
    row.appendChild(msg);
    el.log.appendChild(row);
    while (el.log.children.length > 80) el.log.removeChild(el.log.firstChild);
    el.log.scrollTop = el.log.scrollHeight;
  }

  function render() {
    el.stState.textContent = sim.mode;
    el.stMotion.textContent = sim.moving ? 'em movimento' : 'parado';
    el.stAxis.textContent = String(sim.axis);
    el.stTarget.textContent = String(sim.targetX);
    el.stSeq.textContent = String(sim.seq);
    el.stLink.textContent = linkCut ? 'cortado' : sim.linkLost ? 'perdido' : 'ativo';
    el.stFault.textContent = sim.fault;
    el.stAck.textContent = Core.telemetry(sim);
    el.caption.textContent = 'Posição comandada: ' + sim.axis +
      ' passos · alvo x: ' + sim.targetX + ' · zona morta ±' + SIM.DEAD_BAND + '.';
    el.marker.style.left = (50 + (sim.axis / SIM.MAX_TRAVEL) * 42) + '%';

    el.btnTrack.textContent = 'Rastreamento: ' + (sim.mode === 'TRACK' ? 'ON' : 'OFF');
    el.btnTrack.setAttribute('aria-pressed', String(sim.mode === 'TRACK'));
    el.btnLink.textContent = linkCut ? 'Restaurar link' : 'Simular perda de link';
    el.btnLink.setAttribute('aria-pressed', String(linkCut));
    el.btnRange.textContent = 'Falha Range: ' + (sim.fault === 'Range' ? 'ON' : 'OFF');
    el.btnRange.setAttribute('aria-pressed', String(sim.fault === 'Range'));

    const faulted = sim.fault !== 'None';
    el.header.textContent = faulted ? 'Falha: ' + sim.fault : 'Simulação ativa';
    el.header.classList.toggle('is-fault', faulted);
  }

  function send(line) {
    const accepted = Core.feed(sim, line, clock());
    addLog('tx', line);
    if (accepted) {
      addLog('rx', Core.telemetry(sim));
    } else {
      addLog('evt', 'REJEITADO: ' + line + ' — ' + Core.lastReason() +
        (Core.lastReason() === 'range' ? ' (fault=Range)' : ''));
    }
    render();
    return accepted;
  }

  function setTracking(on) {
    if (on) nextSeq = Math.max(nextSeq, sim.seq + 1);
    send('TRACK,' + (on ? 1 : 0));
  }

  function toggleLink() {
    linkCut = !linkCut;
    if (linkCut) {
      addLog('evt', 'Link cortado (simulação) — watchdog em ' +
        SIM.LINK_TIMEOUT_MS + ' ms');
    } else {
      Core.relink(sim, clock());
      addLog('evt', 'Link restaurado (simulação)');
      addLog('rx', Core.telemetry(sim));
    }
    render();
  }

  function toggleRange() {
    const on = sim.fault !== 'Range';
    Core.setSimFault(sim, on ? 'Range' : 'None');
    addLog('evt', 'Falha Range ' + (on ? 'injetada' : 'removida') + ' (simulação)');
    render();
  }

  function reset() {
    sim = Core.createState();
    sim.lastAcceptedMs = clock();
    nextSeq = 0;
    syntheticTarget = 0;
    linkCut = false;
    addLog('evt', 'BOOT: SAFE, eixo parado, fault None');
    addLog('rx', Core.telemetry(sim));
    render();
  }

  // Gerador de amostras: so roda em TRACK, com link vivo e sequência crescente.
  function tick() {
    const now = clock();
    if (Core.checkWatchdog(sim, now)) {
      addLog('evt', 'WATCHDOG: sem comando aceito em ' + SIM.LINK_TIMEOUT_MS +
        ' ms — SAFE, eixo parado');
      addLog('rx', Core.telemetry(sim));
    }
    if (sim.mode === 'TRACK' && !linkCut) {
      syntheticTarget = Core.clamp(
        syntheticTarget + Math.round((Math.random() - 0.5) * 240), Core.X_LIMIT);
      const valid = Math.random() > 0.15;
      const seq = Math.max(nextSeq, sim.seq + 1);
      send('T,' + seq + ',' + syntheticTarget + ',' + (valid ? 1 : 0));
      nextSeq = seq + 1;
    }
    render();
  }

  function switchTab(name) {
    el.tabs.forEach((tab) => {
      const active = tab.dataset.tab === name;
      tab.classList.toggle('active', active);
      tab.setAttribute('aria-selected', String(active));
    });
    el.panels.forEach((panel) => {
      const active = panel.id === 'tab-' + name;
      panel.classList.toggle('active', active);
      panel.hidden = !active;
    });
  }

  function initTheme() {
    const light = window.matchMedia &&
      window.matchMedia('(prefers-color-scheme: light)').matches;
    document.documentElement.dataset.theme = light ? 'light' : 'dark';
  }

  function fillConstants() {
    const rows = [
      ['MAX_DATAGRAM_BYTES', SIM.MAX_DATAGRAM_BYTES],
      ['LINK_TIMEOUT_MS', SIM.LINK_TIMEOUT_MS],
      ['INT32_MAX', SIM.INT32_MAX],
      ['DEAD_BAND', SIM.DEAD_BAND],
      ['MANUAL_STEP', SIM.MANUAL_STEP],
      ['MAX_TRAVEL', SIM.MAX_TRAVEL],
      ['TRACK_GAIN', SIM.TRACK_GAIN],
      ['CONTROL_PORT', SIM.CONTROL_PORT + ' (previsto, NAO CONFIRMADO)'],
    ];
    rows.forEach(([name, value]) => {
      const tr = document.createElement('tr');
      const tdName = document.createElement('td');
      const tdValue = document.createElement('td');
      tdName.textContent = name;
      tdValue.textContent = String(value);
      tr.appendChild(tdName);
      tr.appendChild(tdValue);
      el.constants.appendChild(tr);
    });
  }

  function bind() {
    el.tabs.forEach((tab) => {
      tab.addEventListener('click', () => switchTab(tab.dataset.tab));
    });
    el.theme.addEventListener('click', () => {
      const root = document.documentElement;
      root.dataset.theme = root.dataset.theme === 'light' ? 'dark' : 'light';
    });
    el.btnTrack.addEventListener('click', () => setTracking(sim.mode !== 'TRACK'));
    document.querySelectorAll('[data-manual]').forEach((button) => {
      button.addEventListener('click', () => send('MANUAL,' + button.dataset.manual));
    });
    el.btnLink.addEventListener('click', toggleLink);
    el.btnRange.addEventListener('click', toggleRange);
    el.btnReset.addEventListener('click', reset);
    el.form.addEventListener('submit', (event) => {
      event.preventDefault();
      const line = el.cmd.value.trim();
      if (line) send(line);
    });
  }

  function init() {
    initTheme();
    bind();
    fillConstants();
    sim.lastAcceptedMs = clock();
    addLog('evt', 'BOOT: SAFE, eixo parado, fault None');
    addLog('rx', Core.telemetry(sim));
    render();
    window.setInterval(tick, 250);
  }

  if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', init);
  } else {
    init();
  }
})(typeof globalThis !== 'undefined' ? globalThis : this);
