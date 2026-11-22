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
    // Rejeicao por faixa e diagnostico; Lnk e dominante ate T aceito.
    if (reason === 'range' && !s.linkLost) s.fault = 'Range';
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
      if (s.linkLost) return true;  // valido, mas inibido ate T aceito
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
      if (s.linkLost) return true;  // valido, mas inibido ate T aceito
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

  // --------------------------------------------------- saida STEP/DIR ----
  // Camada trivial e isolada: um pulso por update(), intervalo recalculado
  // pela velocidade, rampa limitada por aceleracao e distancia de frenagem.
  // Mesmo logica pura: nenhum pino e tocado ate `gate` ser verdadeiro.
  const STEP_DIR = Object.freeze({
    FULL_STEPS_PER_REV: 200,
    STEP_PULSE_US: 10,
    DIR_SETUP_US: 5,
    DEFAULT_SPEED: 800,
    DEFAULT_ACCEL: 4000,
    SPEED_MAX: 10000,
    ACCEL_MAX: 100000,
    PULSE_TEST_MAX: 1000,
  });

  function createStepDir(onStep, onDir) {
    return {
      onStep: onStep || null,
      onDir: onDir || null,
      stepWrites: 0,
      dirWrites: 0,
      stepHighWrites: 0,
      stepsPerRev: STEP_DIR.FULL_STEPS_PER_REV * 16,
      speed: STEP_DIR.DEFAULT_SPEED,
      accel: STEP_DIR.DEFAULT_ACCEL,
      gate: false,
      target: 0,
      position: 0,
      direction: 0,
      pulses: 0,
      velocity: 0,
      started: false,
      lastStepUs: 0,
      lastUpdateUs: 0,
      dirReadyUs: 0,
      pulseUntilUs: 0,
      stepLevel: false,
      dirLevel: false,
      testMode: false,
    };
  }

  function writeStep(d, level) {
    d.stepWrites += 1;
    if (level) d.stepHighWrites += 1;
    if (d.onStep) d.onStep(level);
  }

  function writeDir(d, level) {
    d.dirWrites += 1;
    if (d.onDir) d.onDir(level);
  }

  function stepDirTestRemaining(d) {
    return d.testMode ? Math.abs(d.target - d.position) : 0;
  }

  function setStepDirTarget(d, target) {
    if (d.testMode) return;  // o ensaio tem prioridade sobre a FSM
    d.target = target;
  }

  function setStepDirMicrostep(d, value) {
    if (value !== 8 && value !== 16) return false;
    if (d.velocity !== 0 || d.stepLevel || d.testMode) return false;
    d.stepsPerRev = STEP_DIR.FULL_STEPS_PER_REV * value;
    return true;
  }

  function setStepDirSpeed(d, value) {
    if (!Number.isSafeInteger(value) || value < 1 ||
        value > STEP_DIR.SPEED_MAX) return false;
    d.speed = value;
    return true;
  }

  function setStepDirAccel(d, value) {
    if (!Number.isSafeInteger(value) || value < 1 ||
        value > STEP_DIR.ACCEL_MAX) return false;
    d.accel = value;
    return true;
  }

  function setStepDirGate(d, value) {
    if (d.gate === value) return;
    d.gate = value;
    if (!value) {
      d.velocity = 0;
      d.testMode = false;  // fechar gate aborta qualquer ensaio em curso
      d.target = d.position;
      if (d.stepLevel) {
        d.stepLevel = false;
        writeStep(d, false);
      }
    }
  }

  // N pulsos em uma direcao; exige gate aberto e eixo em repouso.
  function stepDirTestPulse(d, steps, direction) {
    if (!Number.isSafeInteger(steps) || steps < 1 ||
        steps > STEP_DIR.PULSE_TEST_MAX) return false;
    if (direction !== 1 && direction !== -1) return false;
    if (!d.gate || d.testMode || d.velocity !== 0 || d.stepLevel) return false;
    const destination = d.position + steps * direction;
    if (destination < -2147483648 || destination > 2147483647) return false;
    d.target = destination;
    d.testMode = true;
    d.velocity = 0;
    return true;
  }

  function stepDirUpdate(d, nowUs) {
    if (!d.started) {
      d.started = true;
      d.lastUpdateUs = nowUs;
      d.lastStepUs = nowUs;
    }
    if (d.stepLevel && nowUs >= d.pulseUntilUs) {
      d.stepLevel = false;
      writeStep(d, false);
    }
    if (!d.gate) {
      d.velocity = 0;
      d.lastUpdateUs = nowUs;
      return 0;
    }
    const elapsed = nowUs - d.lastUpdateUs;
    const dt = elapsed > 0 ? elapsed / 1e6 : 0;
    d.lastUpdateUs = nowUs;

    const remaining = d.target - d.position;
    if (remaining === 0) {
      d.velocity = 0;
      if (d.testMode) {
        d.testMode = false;
        d.target = d.position;  // devolve o alvo a quem manda depois do ensaio
      }
      return 0;
    }
    const sign = remaining > 0 ? 1 : -1;
    if (sign !== d.direction) {
      d.direction = sign;
      d.dirLevel = sign > 0;
      d.dirReadyUs = nowUs + STEP_DIR.DIR_SETUP_US;
      writeDir(d, d.dirLevel);
    }
    if (nowUs < d.dirReadyUs) return 0;

    const dist = Math.abs(remaining);
    const vLimit = Math.sqrt(2 * d.accel * dist);
    const wanted = sign * Math.min(d.speed, vLimit);
    const delta = d.accel * dt;
    if (wanted > d.velocity) {
      d.velocity = Math.min(wanted, d.velocity + delta);
    } else if (wanted < d.velocity) {
      d.velocity = Math.max(wanted, d.velocity - delta);
    }
    const speedNow = Math.abs(d.velocity);
    if (speedNow < 1e-9) return 0;
    const interval = 1e6 / speedNow;
    if (nowUs - d.lastStepUs < interval) return 0;

    d.lastStepUs = nowUs;
    d.position += sign;
    d.pulses += 1;
    d.stepLevel = true;
    d.pulseUntilUs = nowUs + STEP_DIR.STEP_PULSE_US;
    writeStep(d, true);
    return 1;
  }

  function stepDirConfig(d, line) {
    if (line === 'CFG,?') {
      return 'CFG,steps=' + d.stepsPerRev + ',speed=' + d.speed +
        ',accel=' + d.accel + ',gate=' + (d.gate ? 1 : 0) +
        ',test=' + stepDirTestRemaining(d);
    }
    if (line === 'CFG,GATE,0') {
      setStepDirGate(d, false);
      return 'CFG,GATE,0,OK';
    }
    if (line === 'CFG,GATE,1,CONFIRM') {
      setStepDirGate(d, true);
      return 'CFG,GATE,1,OK';
    }
    if (line.indexOf('CFG,GATE,') === 0) return 'REASON,format';

    if (line.indexOf('CFG,PULSE,') === 0) {
      const match = /^([0-9]+),([+-]?)1$/.exec(line.slice(10));
      if (!match) return 'REASON,format';
      const steps = Number(match[1]);
      if (steps < 1 || steps > STEP_DIR.PULSE_TEST_MAX) return 'REASON,format';
      const direction = match[2] === '-' ? -1 : 1;
      if (!d.gate) return 'REASON,gate';
      if (!stepDirTestPulse(d, steps, direction)) return 'REASON,busy';
      return 'CFG,PULSE,' + steps + ',' + direction + ',OK';
    }
    if (line.indexOf('CFG,PULSE') === 0) return 'REASON,format';

    const keys = [
      ['MICRO', 'CFG,MICRO,', setStepDirMicrostep],
      ['SPEED', 'CFG,SPEED,', setStepDirSpeed],
      ['ACCEL', 'CFG,ACCEL,', setStepDirAccel],
    ];
    for (let i = 0; i < keys.length; i += 1) {
      const key = keys[i][0];
      const prefix = keys[i][1];
      const setter = keys[i][2];
      if (line.indexOf(prefix) !== 0) continue;
      const tail = line.slice(prefix.length);
      if (!/^[0-9]+$/.test(tail)) return 'REASON,format';
      const value = Number(tail);
      if (key === 'MICRO' && value !== 8 && value !== 16) return 'REASON,range';
      if (!setter(d, value)) {
        return key === 'MICRO' ? 'REASON,busy' : 'REASON,range';
      }
      return 'CFG,' + key + ',' + value + ',OK';
    }
    return 'REASON,format';
  }

  function stepDirSnapshot(d) {
    return {
      steps_per_rev: d.stepsPerRev,
      speed: d.speed,
      accel: d.accel,
      gate: d.gate,
      target: d.target,
      position: d.position,
      direction: d.direction,
      pulses: d.pulses,
      step: d.stepLevel,
      dir: d.dirLevel,
      test: d.testMode,
      test_remaining: stepDirTestRemaining(d),
      step_writes: d.stepWrites,
      dir_writes: d.dirWrites,
      step_high_writes: d.stepHighWrites,
    };
  }

  // ------------------------------------------------ deteccao sintetica ----
  // Geometria do laco fechado, somente logica: a pessoa tem um bearing no
  // mundo, a camera gira junto com o duto (pan em passos) e o erro VISIVEL e
  // a diferenca entre os dois. Nenhum quadro real e capturado: nao ha camera,
  // GPIO, driver nem motor. O alvo de pan e apenas um numero para a UI.
  const CAM = Object.freeze({
    FRAME_W: 320,
    FRAME_H: 240,
    CENTER_BAND_PX: 32,
    PAN_X_SCALE: X_LIMIT / SIM.MAX_TRAVEL,
  });

  // Webcam local da UI: so amostra o quadro e estima um x do contrato.
  // Nao e detector de pessoa, nao grava e nao envia nada para o firmware.
  const WEBCAM = Object.freeze({
    SAMPLE_W: 40,
    SAMPLE_H: 30,
    DIFF_THRESHOLD: 18,
    MIN_FOREGROUND_RATIO: 0.01,
    MAX_FOREGROUND_RATIO: 0.55,
  });

  // Centro de massa dos pixels que mudaram em relacao ao fundo. Funcao pura:
  // recebe arrays de cinza e devolve x em [-1000, 1000] ou null (sem alvo).
  function webcamForegroundX(background, frame, width, height, threshold) {
    if (!background || !frame || !Number.isInteger(width) ||
        !Number.isInteger(height) || width < 2 || height < 2) return null;
    const total = width * height;
    if (background.length < total || frame.length < total ||
        !Number.isFinite(threshold)) return null;
    let weightSum = 0;
    let xSum = 0;
    let changed = 0;
    for (let y = 0; y < height; y += 1) {
      for (let x = 0; x < width; x += 1) {
        const i = y * width + x;
        const diff = Math.abs(frame[i] - background[i]);
        if (diff <= threshold) continue;
        const weight = diff - threshold;
        weightSum += weight;
        xSum += x * weight;
        changed += 1;
      }
    }
    if (weightSum === 0 || changed === 0) return null;
    const ratio = changed / total;
    // Mudanca global (luz/exposicao) ou ruido isolado nao e alvo valido.
    if (ratio < WEBCAM.MIN_FOREGROUND_RATIO ||
        ratio > WEBCAM.MAX_FOREGROUND_RATIO) return null;
    const centroidX = xSum / weightSum;
    const x = Math.round((centroidX / (width - 1) * 2 - 1) * X_LIMIT);
    return clamp(x, X_LIMIT);
  }

  function createDetection() {
    const d = {
      bearingX: 0,   // bearing da pessoa no mundo, em x do contrato
      panSteps: 0,   // pan da camera (simulado), em passos do eixo
      seen: true,    // flag sintetica de deteccao valida
      view: null,
    };
    detectionUpdate(d);
    return d;
  }

  function detectionSetBearing(d, x) {
    if (!Number.isFinite(x)) return false;
    d.bearingX = clamp(Math.round(x), X_LIMIT);
    detectionUpdate(d);
    return true;
  }

  function detectionSetPan(d, steps) {
    if (!Number.isFinite(steps)) return false;
    d.panSteps = clamp(Math.round(steps), SIM.MAX_TRAVEL);
    detectionUpdate(d);
    return true;
  }

  function detectionSetSeen(d, seen) {
    d.seen = Boolean(seen);
    detectionUpdate(d);
    return true;
  }

  function detectionUpdate(d) {
    const panX = clamp(Math.round(d.panSteps * CAM.PAN_X_SCALE), X_LIMIT);
    const apparentRaw = d.bearingX - panX;
    const detected = d.seen && Math.abs(apparentRaw) <= X_LIMIT;
    const apparentX = clamp(apparentRaw, X_LIMIT);
    const markerPx = Math.round(apparentX / X_LIMIT * (CAM.FRAME_W / 2));
    const panErrorSteps = detected
      ? trackSteps(apparentX, SIM.MAX_TRAVEL, SIM.TRACK_GAIN, SIM.DEAD_BAND)
      : 0;
    d.view = {
      panX,
      apparentX,
      detected,
      errorX: detected ? apparentX : 0,
      errorPx: detected ? markerPx : 0,
      panErrorSteps,
      panTargetSteps: clamp(d.panSteps + panErrorSteps, SIM.MAX_TRAVEL),
      pixelX: CAM.FRAME_W / 2 + markerPx,
    };
    return d.view;
  }

  // Servo sintetico: aplica o alvo de pan calculado. Nao escreve pino algum.
  function detectionCloseLoop(d) {
    const target = d.view.panTargetSteps;
    if (target === d.panSteps) return false;
    d.panSteps = target;
    detectionUpdate(d);
    return true;
  }

  function detectionSnapshot(d) {
    return {
      bearing_x: d.bearingX,
      pan_steps: d.panSteps,
      seen: d.seen,
      pan_x: d.view.panX,
      apparent_x: d.view.apparentX,
      detected: d.view.detected,
      error_x: d.view.errorX,
      error_px: d.view.errorPx,
      pan_error_steps: d.view.panErrorSteps,
      pan_target_steps: d.view.panTargetSteps,
      pixel_x: d.view.pixelX,
    };
  }

  const Core = Object.freeze({
    SIM, STATES, FAULTS, X_LIMIT,
    byteLength, clamp, trackSteps, createState, telemetry, linkExpired,
    feed, lastReason: lastReasonValue, parseDecision,
    checkWatchdog, relink, setSimFault,
    STEP_DIR, createStepDir, setStepDirTarget, setStepDirGate,
    stepDirTestPulse, stepDirUpdate, stepDirConfig, stepDirSnapshot,
    CAM, createDetection, detectionSetBearing, detectionSetPan,
    detectionSetSeen, detectionUpdate, detectionCloseLoop,
    detectionSnapshot, WEBCAM, webcamForegroundX,
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
    cfgForm: $('cfg-form'), cfgSpeed: $('cfg-speed'), cfgAccel: $('cfg-accel'),
    cfgMicro: $('cfg-micro'), cfgQuery: $('cfg-query'), cfgReply: $('cfg-reply'),
    outGate: $('out-gate'), outSource: $('out-source'), outSteps: $('out-steps'),
    outVelocity: $('out-velocity'), outTarget: $('out-target'),
    outPosition: $('out-position'), outPulses: $('out-pulses'),
    outLevels: $('out-levels'), outTest: $('out-test'),
    gateOpen: $('gate-open'), gateClose: $('gate-close'),
    pulseForm: $('pulse-form'), pulseCount: $('pulse-count'), pulseDir: $('pulse-dir'),
    gateDialog: $('gate-dialog'), gateCancel: $('gate-dialog-cancel'),
    gateConfirm: $('gate-dialog-confirm'),
    detPerson: $('cam-person'), detLost: $('cam-lost'),
    detStatus: $('det-status'), detBearing: $('det-bearing'),
    detPan: $('det-pan'), detPanX: $('det-pan-x'),
    detApparent: $('det-apparent'), detError: $('det-error'),
    detErrorPx: $('det-error-px'), detTarget: $('det-target'),
    detBearingSlider: $('det-bearing-slider'), detPanSlider: $('det-pan-slider'),
    detBearingValue: $('det-bearing-value'), detPanValue: $('det-pan-value'),
    detSeen: $('det-seen'), detClosedLoop: $('det-closed-loop'),
    camFrame: $('cam-frame'), camVideo: $('cam-video'),
    camStart: $('cam-start'), camCalibrate: $('cam-calibrate'),
    camStop: $('cam-stop'), camStatus: $('cam-status'),
  };

  let sim = Core.createState();
  let nextSeq = 0;
  let syntheticTarget = 0;
  let linkCut = false;
  let stepDir = Core.createStepDir();
  let gateSource = 'nenhuma';
  let cfgReply = '—';
  let detection = Core.createDetection();

  // Estado somente da UI: stream local, fundo calibrado e timer de amostragem.
  const webcam = {
    stream: null,
    timer: null,
    canvas: null,
    ctx: null,
    background: null,
    active: false,
    lastX: null,
  };

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

  function clockUs() {
    return Math.round(clock() * 1000);
  }

  // Plano de bancada `CFG,...`: nao passa pelo parser validado do protocolo
  // e nao altera estado/fault/sequencia da sessao.
  function applyCfg(line, origin) {
    addLog('tx', line + (origin ? ' [' + origin + ']' : ''));
    cfgReply = Core.stepDirConfig(stepDir, line);
    addLog('rx', cfgReply);
    renderSaida();
    return cfgReply;
  }

  function closeGate(origin) {
    if (!stepDir.gate) return;
    applyCfg('CFG,GATE,0', origin);
    gateSource = 'nenhuma';
  }

  function openGate() {
    if (linkCut || sim.linkLost || sim.fault !== 'None') {
      addLog('evt', 'Gate recusado: link/falha ativa — restaure a sessão');
      renderSaida();
      return;
    }
    applyCfg('CFG,GATE,1,CONFIRM', 'WEB');
    gateSource = stepDir.gate ? 'WEB' : 'nenhuma';
    addLog('evt', stepDir.gate
      ? 'Gate WEB aberto (somente lógico — nenhum GPIO tocado)'
      : 'Gate WEB recusado pela camada de saída');
    renderSaida();
  }

  function renderSaida() {
    if (!el.outGate) return;
    const snap = Core.stepDirSnapshot(stepDir);
    el.outGate.textContent = stepDir.gate ? 'ON' : 'OFF';
    el.outSource.textContent = gateSource;
    el.outSteps.textContent = String(snap.steps_per_rev);
    el.outVelocity.textContent = String(Math.round(Math.abs(stepDir.velocity)));
    el.outTarget.textContent = String(snap.target);
    el.outPosition.textContent = String(snap.position);
    el.outPulses.textContent = String(snap.pulses);
    el.outLevels.textContent = (snap.dir >= 0 ? '+' : '−') + ' / ' +
      (snap.step ? 'alto' : 'baixo');
    el.outTest.textContent = snap.test
      ? 'ativo (' + snap.test_remaining + ')' : '—';
    el.cfgReply.textContent = 'Última resposta: ' + cfgReply;
    el.gateOpen.setAttribute('aria-pressed', String(stepDir.gate));
    el.gateOpen.textContent = stepDir.gate
      ? 'Saída liberada (WEB)' : 'Liberar saída (WEB)';
  }

  function setWebcamStatus(text) {
    if (el.camStatus) el.camStatus.textContent = text;
  }

  function renderWebcamControls() {
    if (!el.camStart) return;
    el.camStart.setAttribute('aria-pressed', String(webcam.active));
    el.camStart.disabled = webcam.active;
    el.camCalibrate.disabled = !webcam.active;
    el.camStop.disabled = !webcam.active;
    el.camStart.textContent = webcam.active ? 'Webcam ativa' : 'Ativar webcam';
    // Enquanto a webcam manda o bearing, os controles sintéticos seriam
    // sobrescritos no próximo quadro: desabilita para não enganar quem usa.
    for (const controles of [el.detBearingSlider, el.detPanSlider,
                             el.detSeen, el.detClosedLoop]) {
      if (controles) controles.disabled = webcam.active;
    }
    if (el.camFrame) {
      el.camFrame.classList.toggle('webcam-on', webcam.active);
      el.camFrame.setAttribute('aria-label', webcam.active
        ? 'Webcam local ao vivo com marcador do proxy de fundo e zona central'
        : 'Quadro sintético com a pessoa e a zona central');
    }
    if (el.camVideo) el.camVideo.hidden = !webcam.active;
  }

  function stopWebcam(reason) {
    if (webcam.timer !== null) {
      window.clearInterval(webcam.timer);
      webcam.timer = null;
    }
    if (webcam.stream) {
      webcam.stream.getTracks().forEach((track) => track.stop());
      webcam.stream = null;
    }
    if (el.camVideo) el.camVideo.srcObject = null;
    webcam.active = false;
    webcam.background = null;
    webcam.lastX = null;
    Core.detectionSetSeen(detection, el.detSeen ? el.detSeen.checked : true);
    renderWebcamControls();
    renderDeteccao();
    setWebcamStatus(reason || 'Webcam desligada — quadro sintético em uso.');
  }

  function grayFrame(imageData) {
    const count = imageData.width * imageData.height;
    const gray = new Uint8Array(count);
    for (let i = 0; i < count; i += 1) {
      const p = i * 4;
      gray[i] = Math.round(imageData.data[p] * 0.299 +
        imageData.data[p + 1] * 0.587 + imageData.data[p + 2] * 0.114);
    }
    return gray;
  }

  function sampleWebcam() {
    if (!webcam.active || !webcam.ctx || !el.camVideo ||
        el.camVideo.readyState < 2) return null;
    webcam.ctx.drawImage(el.camVideo, 0, 0,
      Core.WEBCAM.SAMPLE_W, Core.WEBCAM.SAMPLE_H);
    return webcam.ctx.getImageData(0, 0,
      Core.WEBCAM.SAMPLE_W, Core.WEBCAM.SAMPLE_H);
  }

  function analyzeWebcamFrame() {
    const sample = sampleWebcam();
    if (!sample) return;
    const frame = grayFrame(sample);
    if (!webcam.background) {
      webcam.background = frame;
      setWebcamStatus('Webcam ativa: fundo capturado. Entre no quadro ou '
        + 'use "Calibrar fundo" com a cena vazia.');
      return;
    }
    const x = Core.webcamForegroundX(webcam.background, frame,
      Core.WEBCAM.SAMPLE_W, Core.WEBCAM.SAMPLE_H,
      Core.WEBCAM.DIFF_THRESHOLD);
    if (x === null) {
      webcam.lastX = null;
      Core.detectionSetSeen(detection, false);
      setWebcamStatus('Webcam ativa — sem alvo no proxy local (luz/ruído '
        + 'ou cena igual ao fundo); nenhuma amostra T é gerada.');
      renderDeteccao();
      return;
    }
    webcam.lastX = x;
    Core.detectionSetBearing(detection, x);
    Core.detectionSetSeen(detection, true);
    setWebcamStatus('Webcam ativa — x estimado: ' + x +
      ' (proxy de fundo, não é detector de pessoa).');
    renderDeteccao();
  }

  function calibrateWebcam() {
    const sample = sampleWebcam();
    if (!sample) {
      setWebcamStatus('Não foi possível calibrar: aguarde o vídeo ficar pronto.');
      return false;
    }
    webcam.background = grayFrame(sample);
    webcam.lastX = null;
    Core.detectionSetSeen(detection, false);
    renderDeteccao();
    setWebcamStatus('Fundo recalibrado com o quadro atual — nada é enviado '
      + 'ao firmware.');
    return true;
  }

  async function startWebcam() {
    if (webcam.active) return;
    if (!navigator.mediaDevices || !navigator.mediaDevices.getUserMedia) {
      setWebcamStatus('Webcam indisponível neste contexto. Use um navegador '
        + 'com suporte e origem segura (file:// ou localhost).');
      return;
    }
    try {
      const stream = await navigator.mediaDevices.getUserMedia({
        audio: false,
        video: { facingMode: 'user', width: { ideal: 640 }, height: { ideal: 480 } },
      });
      webcam.stream = stream;
      if (el.camVideo) el.camVideo.srcObject = stream;
      if (el.camVideo) await el.camVideo.play();
      webcam.canvas = document.createElement('canvas');
      webcam.canvas.width = Core.WEBCAM.SAMPLE_W;
      webcam.canvas.height = Core.WEBCAM.SAMPLE_H;
      webcam.ctx = webcam.canvas.getContext('2d', { willReadFrequently: true });
      if (!webcam.ctx) throw new Error('canvas indisponível');
      webcam.active = true;
      webcam.background = null;
      webcam.lastX = null;
      Core.detectionSetSeen(detection, false);
      renderWebcamControls();
      renderDeteccao();
      setWebcamStatus('Webcam ativa — capturando o fundo neste navegador.');
      webcam.timer = window.setInterval(analyzeWebcamFrame, 150);
      analyzeWebcamFrame();
    } catch (error) {
      const nome = error && error.name ? error.name : 'erro desconhecido';
      stopWebcam('Webcam não iniciada (' + nome +
        '). Permissão negada ou dispositivo sem câmera: o quadro sintético '
        + 'continua disponível.');
    }
  }

  function renderDeteccao() {
    if (!el.detPerson) return;
    const v = Core.detectionSnapshot(detection);
    el.detPerson.style.left =
      (v.pixel_x / CAM.FRAME_W * 100) + '%';
    el.detPerson.hidden = !v.detected;
    el.detLost.hidden = v.detected;
    el.detStatus.textContent = v.detected ? 'detectada' : 'sem detecção';
    el.detBearing.textContent = String(v.bearing_x);
    el.detPan.textContent = String(v.pan_steps);
    el.detPanX.textContent = String(v.pan_x);
    el.detApparent.textContent = String(v.apparent_x);
    el.detError.textContent = String(v.error_x);
    el.detErrorPx.textContent = String(v.error_px);
    el.detTarget.textContent = String(v.pan_target_steps);
    if (el.detBearingValue) el.detBearingValue.textContent = String(v.bearing_x);
    if (el.detPanValue) el.detPanValue.textContent = String(v.pan_steps);
    if (el.detBearingSlider.value !== String(v.bearing_x)) {
      el.detBearingSlider.value = String(v.bearing_x);
    }
    if (el.detPanSlider.value !== String(v.pan_steps)) {
      el.detPanSlider.value = String(v.pan_steps);
    }
    el.detSeen.checked = v.seen;
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
    renderSaida();
    renderDeteccao();
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
      closeGate('link');
      addLog('evt', 'Link cortado (simulação) — watchdog em ' +
        SIM.LINK_TIMEOUT_MS + ' ms, gate fechado');
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
    closeGate('reset');
    stopWebcam('Webcam desligada no reinício — quadro sintético em uso.');
    sim = Core.createState();
    sim.lastAcceptedMs = clock();
    nextSeq = 0;
    syntheticTarget = 0;
    linkCut = false;
    stepDir = Core.createStepDir();
    gateSource = 'nenhuma';
    cfgReply = '—';
    detection = Core.createDetection();
    addLog('evt', 'BOOT: SAFE, eixo parado, fault None, gate OFF');
    addLog('rx', Core.telemetry(sim));
    render();
  }

  // Gerador de amostras: so roda em TRACK, com link vivo e sequência crescente.
  function tick() {
    const now = clock();
    if (Core.checkWatchdog(sim, now)) {
      closeGate('watchdog');
      addLog('evt', 'WATCHDOG: sem comando aceito em ' + SIM.LINK_TIMEOUT_MS +
        ' ms — SAFE, eixo parado, gate fechado');
      addLog('rx', Core.telemetry(sim));
    }
    // Com webcam, o pan exibido e o eixo comandado pela FSM; o servo sintetico
    // fica fora para nao competir com o mesmo alvo.
    if (webcam.active && webcam.lastX !== null) {
      Core.detectionSetPan(detection, sim.axis);
    }
    // Com webcam ativa e sem alvo, nao inventa amostra T: eixo para no SAFE.
    if (sim.mode === 'TRACK' && !linkCut &&
        !(webcam.active && webcam.lastX === null)) {
      const usarWebcam = webcam.active && webcam.lastX !== null;
      if (usarWebcam) {
        syntheticTarget = webcam.lastX;  // proxy local no lugar do alvo aleatorio
      } else {
        syntheticTarget = Core.clamp(
          syntheticTarget + Math.round((Math.random() - 0.5) * 240), Core.X_LIMIT);
      }
      const valid = usarWebcam || Math.random() > 0.15;
      const seq = Math.max(nextSeq, sim.seq + 1);
      send('T,' + seq + ',' + syntheticTarget + ',' + (valid ? 1 : 0));
      nextSeq = seq + 1;
    }

    // O eixo comandado pela FSM e o alvo da camada de saida. Nenhum pino e
    // escrito: tudo permanece no estado logico do navegador.
    Core.setStepDirTarget(stepDir, sim.axis);
    Core.stepDirUpdate(stepDir, clockUs());
    if (!webcam.active && el.detClosedLoop && el.detClosedLoop.checked) {
      Core.detectionCloseLoop(detection);  // servo sintetico, sem tocar GPIO
    }
    render();
  }

  function switchTab(name, focus) {
    el.tabs.forEach((tab) => {
      const active = tab.dataset.tab === name;
      tab.classList.toggle('active', active);
      tab.setAttribute('aria-selected', String(active));
      tab.tabIndex = active ? 0 : -1;
      if (active && focus) tab.focus();
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
      ['FULL_STEPS_PER_REV', Core.STEP_DIR.FULL_STEPS_PER_REV],
      ['STEP_PULSE_US', Core.STEP_DIR.STEP_PULSE_US],
      ['DIR_SETUP_US', Core.STEP_DIR.DIR_SETUP_US],
      ['SPEED_MAX', Core.STEP_DIR.SPEED_MAX],
      ['ACCEL_MAX', Core.STEP_DIR.ACCEL_MAX],
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
      tab.addEventListener('click', () => switchTab(tab.dataset.tab, true));
    });
    const tablist = document.querySelector('[role="tablist"]');
    tablist.addEventListener('keydown', (event) => {
      if (['ArrowRight', 'ArrowLeft', 'Home', 'End'].indexOf(event.key) < 0) {
        return;
      }
      event.preventDefault();
      const current = Math.max(0, el.tabs.indexOf(document.activeElement));
      let next = current;
      if (event.key === 'ArrowRight') next = (current + 1) % el.tabs.length;
      if (event.key === 'ArrowLeft') {
        next = (current - 1 + el.tabs.length) % el.tabs.length;
      }
      if (event.key === 'Home') next = 0;
      if (event.key === 'End') next = el.tabs.length - 1;
      switchTab(el.tabs[next].dataset.tab, true);
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

    el.cfgForm.addEventListener('submit', (event) => {
      event.preventDefault();
      const speed = Number(el.cfgSpeed.value);
      const accel = Number(el.cfgAccel.value);
      const micro = Number(el.cfgMicro.value);
      if (!Number.isSafeInteger(speed) || speed < 1 || speed > Core.STEP_DIR.SPEED_MAX) {
        addLog('evt', 'Velocidade fora de faixa (1..' + Core.STEP_DIR.SPEED_MAX + ')');
        return;
      }
      if (!Number.isSafeInteger(accel) || accel < 1 || accel > Core.STEP_DIR.ACCEL_MAX) {
        addLog('evt', 'Aceleração fora de faixa (1..' + Core.STEP_DIR.ACCEL_MAX + ')');
        return;
      }
      const replies = [
        applyCfg('CFG,SPEED,' + speed, 'UI'),
        applyCfg('CFG,ACCEL,' + accel, 'UI'),
        applyCfg('CFG,MICRO,' + micro, 'UI'),
      ];
      if (replies.some((reply) => reply.indexOf('REASON') === 0)) {
        addLog('evt', 'Configuração parcialmente recusada — ver respostas acima');
      }
    });
    el.cfgQuery.addEventListener('click', () => applyCfg('CFG,?', 'UI'));

    el.pulseForm.addEventListener('submit', (event) => {
      event.preventDefault();
      const count = Number(el.pulseCount.value);
      const direction = Number(el.pulseDir.value);
      if (!Number.isSafeInteger(count) || count < 1 ||
          count > Core.STEP_DIR.PULSE_TEST_MAX) {
        addLog('evt', 'Ensaio fora de faixa (1..' +
          Core.STEP_DIR.PULSE_TEST_MAX + ' pulsos)');
        return;
      }
      if (direction !== 1 && direction !== -1) {
        addLog('evt', 'Direção de ensaio inválida');
        return;
      }
      if (!stepDir.gate) {
        addLog('evt', 'Ensaio recusado: abra o gate com confirmação primeiro');
        return;
      }
      applyCfg('CFG,PULSE,' + count + ',' + direction, 'UI');
    });

    let gateOpener = null;
    el.gateOpen.addEventListener('click', () => {
      gateOpener = document.activeElement;
      el.gateDialog.showModal();
      el.gateCancel.focus();
    });
    el.gateDialog.addEventListener('close', () => {
      if (gateOpener && gateOpener.isConnected) gateOpener.focus();
      gateOpener = null;
    });
    el.gateCancel.addEventListener('click', () => el.gateDialog.close());
    el.gateConfirm.addEventListener('click', () => {
      el.gateDialog.close();
      openGate();
    });
    el.gateClose.addEventListener('click', () => closeGate('UI'));

    el.detBearingSlider.addEventListener('input', () => {
      Core.detectionSetBearing(detection, Number(el.detBearingSlider.value));
      renderDeteccao();
    });
    el.detPanSlider.addEventListener('input', () => {
      Core.detectionSetPan(detection, Number(el.detPanSlider.value));
      renderDeteccao();
    });
    el.detSeen.addEventListener('change', () => {
      Core.detectionSetSeen(detection, el.detSeen.checked);
      renderDeteccao();
    });
    el.camStart.addEventListener('click', startWebcam);
    el.camCalibrate.addEventListener('click', calibrateWebcam);
    el.camStop.addEventListener('click', () => stopWebcam());
    window.addEventListener('pagehide', () => stopWebcam(
      'Webcam desligada ao sair da página.'));
  }

  function init() {
    initTheme();
    bind();
    fillConstants();
    renderWebcamControls();
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
