/*
 * laserseg — simulador web V1.
 * Replica a experiencia e o contrato UDP v0.1 sem firmware, GPIO ou placa.
 * Fonte da verdade: tools/protocol_ref.py, controller/parser.cpp e
 * docs/PROTOCOLO_UDP.md. Nada aqui e enviado para hardware.
 */
(function () {
  'use strict';

  // Constantes espelhadas do nucleo — tests/test_simulator.py barra divergencia.
  const SIM = {
    MAX_DATAGRAM_BYTES: 96,
    LINK_TIMEOUT_MS: 1200,
    DEAD_BAND: 55,
    MANUAL_STEP: 80,
    MAX_TRAVEL: 533,
    TRACK_GAIN: 0.75,
    STEPS_PER_REV: 3200,
    AP_SSID: 'laserseg',
    AP_PASSWORD: '123456', // excecao publica aprovada em 2026-11-19
    AP_IP: '192.168.4.1',
    CONTROL_PORT: 4210,
  };

  const $ = (id) => document.getElementById(id);

  const el = {
    theme: $('theme-toggle'), header: $('header-status'),
    tabs: Array.from(document.querySelectorAll('.tab')),
    panels: Array.from(document.querySelectorAll('.tab-panel')),
    stState: $('st-state'), stMotion: $('st-motion'), stXY: $('st-xy'),
    stSeq: $('st-seq'), stLink: $('st-link'), stLight: $('st-light'),
    stWifi: $('st-wifi'), stFault: $('st-fault'),
    btnTrack: $('btn-track'), btnLight: $('btn-light'),
    btnLink: $('btn-link-cut'), btnReset: $('btn-reset'),
    btnCamera: $('btn-camera'), cameraMode: $('camera-mode'),
    video: $('cam-video'), canvas: $('cam-canvas'),
    targetDot: $('target-dot'), log: $('log'),
    network: $('wifi-network'), scan: $('btn-scan'),
    ssid: $('wifi-ssid'), pass: $('wifi-pass'), ip: $('ctrl-ip'),
    fail: $('fail-network'), save: $('btn-save-wifi'), apOnly: $('btn-ap-only'),
    prov: $('prov-status'), constants: $('proto-constants'),
  };

  // Estado seguro por padrao: eixos parados, luz OFF, sem falha.
  const state = {
    tracking: false, light: false, axesStopped: true, fault: 'NONE',
    seq: -1, x: 0, y: 0, valid: false, lastPacketMs: null,
    linkCut: false, wifiMode: 'AP', provState: 'kAp', cameraOn: false,
    targetX: 0, targetY: 0, networkPassword: '',
  };

  const networks = ['rede-local-2.4G', 'LAB-WIFI', 'escritorio-5G'];

  // --- Funcoes puras (espelho da referencia host) --------------------------
  function clamp(value, limit) {
    return Math.max(-limit, Math.min(limit, value));
  }

  function targetFromError(error, maxTravel, gain, deadBand) {
    if (Math.abs(error) <= deadBand) return 0;
    return clamp(Math.trunc(error * maxTravel * gain / 1000), maxTravel);
  }

  function manualTarget(current, direction, step) {
    if (direction !== -1 && direction !== 1) return current;
    return clamp(current + direction * step, SIM.MAX_TRAVEL);
  }

  function linkExpired(lastPacketMs, nowMs) {
    return lastPacketMs === null || nowMs - lastPacketMs > SIM.LINK_TIMEOUT_MS;
  }

  function nextState(current, saved, connected) {
    if (!saved) return 'kAp';
    void current; // CURRENT nao altera a decisao nesta V1: falha => AP e para.
    return connected ? 'kSta' : 'kAp';
  }

  function currentMode() {
    if (state.tracking) return 'TRACK';
    return state.axesStopped ? 'SAFE' : 'MANUAL';
  }

  function formatTelemetry() {
    return 'ACK,' + state.seq + ',' + currentMode() + ',' +
      state.fault + ',' + (state.light ? 1 : 0);
  }

  // --- Log e renderizacao --------------------------------------------------
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
    el.stState.textContent = currentMode();
    el.stMotion.textContent = state.valid ? 'detectado' : 'nao detectado';
    el.stXY.textContent = state.x + ' / ' + state.y;
    el.stSeq.textContent = String(state.seq);
    el.stLink.textContent = state.linkCut ? 'cortado' : 'ativo';
    el.stLight.textContent = state.light ? 'ON' : 'OFF';
    el.stWifi.textContent = state.wifiMode;
    el.stFault.textContent = state.fault;
    el.btnTrack.textContent = 'Rastreamento: ' + (state.tracking ? 'ON' : 'OFF');
    el.btnTrack.setAttribute('aria-pressed', String(state.tracking));
    el.btnLight.textContent = 'Luz: ' + (state.light ? 'ON' : 'OFF');
    el.btnLight.setAttribute('aria-pressed', String(state.light));
    el.btnLink.textContent = state.linkCut ? 'Restaurar link' : 'Simular perda de link';
    el.btnLink.setAttribute('aria-pressed', String(state.linkCut));
    const faulted = state.fault !== 'NONE';
    el.header.textContent = faulted ? 'Falha de link' : 'Simulacao ativa';
    el.header.classList.toggle('is-fault', faulted);
    const px = 50 + (state.targetX / 1000) * 45;
    const py = 50 + (state.targetY / 1000) * 45;
    el.targetDot.style.left = px + '%';
    el.targetDot.style.top = py + '%';
    el.targetDot.style.opacity = state.tracking ? '1' : '0.35';
  }

  function drawCanvas() {
    if (state.cameraOn) return;
    const ctx = el.canvas.getContext('2d');
    const w = el.canvas.width;
    const h = el.canvas.height;
    ctx.fillStyle = '#11131a';
    ctx.fillRect(0, 0, w, h);
    ctx.strokeStyle = 'rgba(139, 233, 253, 0.18)';
    ctx.lineWidth = 1;
    for (let i = 1; i < 8; i++) {
      ctx.beginPath();
      ctx.moveTo((w * i) / 8, 0);
      ctx.lineTo((w * i) / 8, h);
      ctx.stroke();
      ctx.beginPath();
      ctx.moveTo(0, (h * i) / 8);
      ctx.lineTo(w, (h * i) / 8);
      ctx.stroke();
    }
  }

  function moveTarget() {
    state.targetX = clamp(state.targetX + Math.round((Math.random() - 0.5) * 220), 1000);
    state.targetY = clamp(state.targetY + Math.round((Math.random() - 0.5) * 140), 1000);
  }

  // --- Comandos do contrato ------------------------------------------------
  function acceptCommand() {
    state.lastPacketMs = Date.now();
    state.fault = 'NONE';
  }

  function setTracking(on) {
    state.tracking = on;
    if (on) {
      state.seq = -1; // TRACK,1 reinicia a janela de sequencia.
      addLog('tx', 'TRACK,1');
    } else {
      addLog('tx', 'TRACK,0');
      state.axesStopped = true;
      state.valid = false;
    }
    acceptCommand();
    addLog('rx', formatTelemetry());
    render();
  }

  function setLight(on) {
    state.light = on;
    addLog('tx', 'LIGHT,' + (on ? 1 : 0));
    acceptCommand();
    addLog('rx', formatTelemetry());
    render();
  }

  function manual(axis, direction) {
    const field = axis === 'pan' ? 'targetX' : 'targetY';
    state[field] = manualTarget(state[field], Number(direction), SIM.MANUAL_STEP);
    state.axesStopped = false;
    addLog('tx', 'MANUAL,' + axis + ',' + direction);
    acceptCommand();
    addLog('rx', formatTelemetry());
    render();
  }

  function reset() {
    state.tracking = false;
    state.light = false;
    state.axesStopped = true;
    state.fault = 'NONE';
    state.seq = -1;
    state.x = 0;
    state.y = 0;
    state.valid = false;
    state.lastPacketMs = null;
    state.linkCut = false;
    addLog('evt', 'RESET: boot seguro — SAFE, eixos parados, luz OFF');
    addLog('rx', formatTelemetry());
    render();
  }

  // --- Ciclo de simulacao --------------------------------------------------
  function tick() {
    const now = Date.now();
    if (state.tracking && !state.linkCut) {
      moveTarget();
      state.valid = Math.random() > 0.15;
      state.x = state.valid ? targetFromError(state.targetX, SIM.MAX_TRAVEL,
        SIM.TRACK_GAIN, SIM.DEAD_BAND) : 0;
      state.y = state.valid ? targetFromError(state.targetY, SIM.MAX_TRAVEL,
        SIM.TRACK_GAIN, SIM.DEAD_BAND) : 0;
      state.seq += 1;
      state.lastPacketMs = now;
      state.fault = 'NONE';
      state.axesStopped = !state.valid;
      addLog('tx', 'T,' + state.seq + ',' + state.x + ',' + state.y + ',' +
        (state.valid ? 1 : 0));
      addLog('rx', formatTelemetry());
    } else if (linkExpired(state.lastPacketMs, now) && state.fault !== 'LINK') {
      state.tracking = false;
      state.light = false;
      state.axesStopped = true;
      state.valid = false;
      state.fault = 'LINK';
      addLog('evt', 'WATCHDOG: sem pacote em ' + SIM.LINK_TIMEOUT_MS +
        ' ms — eixos parados, luz OFF');
      addLog('rx', formatTelemetry());
    }
    drawCanvas();
    render();
  }

  // --- Abas e tema ---------------------------------------------------------
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
    const prefersLight = window.matchMedia &&
      window.matchMedia('(prefers-color-scheme: light)').matches;
    document.documentElement.dataset.theme = prefersLight ? 'light' : 'dark';
  }

  // --- Wi-Fi (provisionamento) --------------------------------------------
  function scanNetworks() {
    el.network.replaceChildren();
    const placeholder = document.createElement('option');
    placeholder.value = '';
    placeholder.textContent = '— escolha uma rede —';
    el.network.appendChild(placeholder);
    networks.forEach((name) => {
      const option = document.createElement('option');
      option.value = name;
      option.textContent = name;
      el.network.appendChild(option);
    });
    addLog('evt', 'PROV SCAN n=' + networks.length);
    el.prov.textContent = 'PROV SCAN n=' + networks.length +
      ' — selecione uma rede.';
  }

  function showAp() {
    state.wifiMode = 'AP';
    state.provState = nextState(state.provState, false, false);
    addLog('evt', 'PROV AP_UP — ' + SIM.AP_SSID + ' / ' + SIM.AP_PASSWORD +
      ' em ' + SIM.AP_IP);
    el.prov.textContent = 'PROV AP_UP — AP ' + SIM.AP_SSID +
      ' disponivel em ' + SIM.AP_IP + '.';
    render();
  }

  function saveWifi() {
    const ssid = el.ssid.value.trim();
    if (!ssid) {
      el.prov.textContent = 'Informe um SSID antes de salvar.';
      return;
    }
    // A senha fica somente nesta variavel, em memoria da pagina.
    state.networkPassword = el.pass.value;
    const controllerIp = el.ip.value.trim();
    addLog('evt', 'PROV SAVED — rede ' + ssid +
      ' (credencial nao registrada no log)');
    el.prov.textContent = 'PROV SAVED — reiniciando para aplicar...';
    el.pass.value = '';
    window.setTimeout(() => {
      const connected = !el.fail.checked;
      state.provState = nextState(state.provState, true, connected);
      if (connected) {
        state.wifiMode = 'STA';
        addLog('evt', 'PROV STA_CONNECTED ip=192.168.1.42 ctrl=' + controllerIp);
        el.prov.textContent = 'PROV STA_CONNECTED — operando em STA.';
      } else {
        state.wifiMode = 'AP';
        addLog('evt', 'PROV FALLBACK_AP — rede ' + ssid +
          ' indisponivel; AP ativo, sem retry em background');
        el.prov.textContent = 'PROV FALLBACK_AP — AP ' + SIM.AP_SSID +
          ' ativo; nenhuma tentativa em background.';
      }
      render();
    }, 600);
  }

  // --- Camera -------------------------------------------------------------
  function toggleCamera() {
    if (state.cameraOn) {
      const stream = el.video.srcObject;
      if (stream) stream.getTracks().forEach((track) => track.stop());
      el.video.srcObject = null;
      el.video.hidden = true;
      el.canvas.hidden = false;
      state.cameraOn = false;
      el.btnCamera.textContent = 'Ativar webcam';
      el.cameraMode.textContent = 'Alvo sintetico ativo';
      return;
    }
    if (!navigator.mediaDevices || !navigator.mediaDevices.getUserMedia) {
      addLog('evt', 'Webcam indisponivel — mantido alvo sintetico.');
      return;
    }
    navigator.mediaDevices.getUserMedia({ video: true })
      .then((stream) => {
        el.video.srcObject = stream;
        el.video.hidden = false;
        el.canvas.hidden = true;
        state.cameraOn = true;
        el.btnCamera.textContent = 'Desativar webcam';
        el.cameraMode.textContent = 'Webcam ativa';
      })
      .catch(() => {
        addLog('evt', 'Webcam nao autorizada — mantido alvo sintetico.');
      });
  }

  // --- Tabela de constantes (aba Protocolo) -------------------------------
  function fillConstants() {
    const rows = [
      ['MAX_DATAGRAM_BYTES', SIM.MAX_DATAGRAM_BYTES],
      ['LINK_TIMEOUT_MS', SIM.LINK_TIMEOUT_MS],
      ['DEAD_BAND', SIM.DEAD_BAND],
      ['MANUAL_STEP', SIM.MANUAL_STEP],
      ['MAX_TRAVEL', SIM.MAX_TRAVEL],
      ['TRACK_GAIN', SIM.TRACK_GAIN],
      ['STEPS_PER_REV', SIM.STEPS_PER_REV],
      ['CONTROL_PORT', SIM.CONTROL_PORT],
      ['AP_SSID', SIM.AP_SSID],
      ['AP_IP', SIM.AP_IP],
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

  // --- Ligacoes ------------------------------------------------------------
  function bind() {
    el.tabs.forEach((tab) => {
      tab.addEventListener('click', () => switchTab(tab.dataset.tab));
    });
    el.theme.addEventListener('click', () => {
      const root = document.documentElement;
      root.dataset.theme = root.dataset.theme === 'light' ? 'dark' : 'light';
    });
    el.btnTrack.addEventListener('click', () => setTracking(!state.tracking));
    el.btnLight.addEventListener('click', () => setLight(!state.light));
    el.btnLink.addEventListener('click', () => {
      state.linkCut = !state.linkCut;
      if (state.linkCut) {
        addLog('evt', 'Link cortado (simulacao)');
      } else {
        acceptCommand();
        addLog('evt', 'Link restaurado — comunicacao reiniciada');
        addLog('rx', formatTelemetry());
      }
      render();
    });
    el.btnReset.addEventListener('click', reset);
    document.querySelectorAll('[data-axis]').forEach((button) => {
      button.addEventListener('click', () => {
        manual(button.dataset.axis, button.dataset.dir);
      });
    });
    el.btnCamera.addEventListener('click', toggleCamera);
    el.scan.addEventListener('click', scanNetworks);
    el.network.addEventListener('change', () => {
      if (el.network.value) el.ssid.value = el.network.value;
    });
    el.save.addEventListener('click', saveWifi);
    el.apOnly.addEventListener('click', showAp);
  }

  function init() {
    initTheme();
    bind();
    fillConstants();
    drawCanvas();
    render();
    addLog('evt', 'BOOT: SAFE, eixos parados, luz OFF');
    addLog('evt', 'PROV AP_UP — ' + SIM.AP_SSID + ' / ' + SIM.AP_PASSWORD);
    el.prov.textContent = 'PROV AP_UP — AP ' + SIM.AP_SSID +
      ' disponivel em ' + SIM.AP_IP + '.';
    window.setInterval(tick, 250);
  }

  if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', init);
  } else {
    init();
  }
})();
