'use strict';

const API = {
  async _try(method, path, body) {
    const opts = { method };
    if (body !== undefined) {
      opts.headers = { 'Content-Type': 'application/json' };
      opts.body = JSON.stringify(body);
    }
    const res = await fetch(path, opts);
    const ct = res.headers.get('content-type') || '';
    const data = ct.includes('application/json') ? await res.json() : await res.text();
    if (!res.ok) throw new Error('HTTP ' + res.status + ' ' + path);
    return data;
  },
  get(path) { return this._try('GET', path); },
  post(path, body) { return this._try('POST', path, body); }
};

const state = { online: false, activeTab: 'home' };
const $ = (id) => document.getElementById(id);
const fmt = (v, d) => (Number.isFinite(v) ? v.toFixed(d) : '--');
const signed = (v, d) => (Number.isFinite(v) ? (v >= 0 ? '+' : '') + v.toFixed(d) : '--');

function uptimeText(ms) {
  if (!Number.isFinite(ms)) return '--';
  const total = Math.floor(ms / 1000);
  const h = Math.floor(total / 3600);
  const m = Math.floor((total % 3600) / 60);
  const s = total % 60;
  return (h ? h + 'h ' : '') + m + 'm ' + s + 's';
}

function setOnline(ok) {
  state.online = !!ok;
  const badge = $('badge-conn');
  if (!badge) return;
  badge.textContent = ok ? 'Conectado' : 'Desconectado';
  badge.className = 'badge ' + (ok ? 'online' : 'offline');
}

function setStatus(message, isError) {
  const el = $('cfg-status');
  if (!el) return;
  el.textContent = message || '';
  el.className = 'cfg-status' + (isError ? ' error' : '');
}

function setTag(id, ok, labels) {
  const el = $(id);
  if (!el) return;
  el.textContent = ok ? labels[0] : labels[1];
  el.className = 'sensor-tag ' + (ok ? 'ok' : 'fail');
}

function renderAlerts(alerts) {
  const box = $('header-alerts');
  if (!box) return;
  const items = [];
  if (alerts && alerts.fire) items.push(['INCÊNDIO', 'crit']);
  if (alerts && alerts.rain) items.push(['Chuva provável', 'warn']);
  if (alerts && alerts.hot) items.push(['Calor', 'warn']);
  if (alerts && alerts.cold) items.push(['Frio', 'info']);
  if (alerts && alerts.dry) items.push(['Ar seco', 'warn']);
  if (!items.length) items.push(['Sem avisos', 'ok']);
  box.innerHTML = items.map(([text, kind]) => '<span class="alert-chip ' + kind + '">' + text + '</span>').join('');
}

function selectTab(name) {
  state.activeTab = name;
  document.querySelectorAll('.tab').forEach((tab) => {
    const active = tab.getAttribute('data-tab') === name;
    tab.classList.toggle('active', active);
    tab.setAttribute('aria-selected', active ? 'true' : 'false');
  });
  ['home', 'config'].forEach((n) => {
    const p = $('panel-' + n);
    if (!p) return;
    p.classList.toggle('active', n === name);
    p.classList.toggle('hidden', n !== name);
  });
  if (name === 'config') {
    loadConfig();
    loadWifi();
  }
  try { history.replaceState(null, '', '#' + name); } catch (e) {}
  try { localStorage.setItem('msa-tab', name); } catch (e) {}
}

function initTabs() {
  document.querySelectorAll('.tab').forEach((tab) => {
    tab.addEventListener('click', () => selectTab(tab.getAttribute('data-tab')));
  });
  let saved = '';
  try { saved = localStorage.getItem('msa-tab') || ''; } catch (e) {}
  const hash = location.hash.replace('#', '');
  selectTab(['home', 'config'].includes(hash) ? hash : (['home', 'config'].includes(saved) ? saved : 'home'));
}

function applyWifiMode() {
  const sel = $('cfg-wifi-mode');
  if (!sel) return;
  const isSta = sel.value === 'sta';
  ['cfg-wifi-sta-row', 'cfg-wifi-pass-row'].forEach((id) => {
    const el = $(id);
    if (el) el.style.display = isSta ? 'flex' : 'none';
  });
  ['cfg-wifi-ap-row', 'cfg-wifi-ap-pass-row'].forEach((id) => {
    const el = $(id);
    if (el) el.style.display = isSta ? 'none' : 'flex';
  });
}

function initWifiMode() {
  const sel = $('cfg-wifi-mode');
  if (!sel) return;
  sel.addEventListener('change', applyWifiMode);
  applyWifiMode();
}

function initFullscreen() {
  const cb = $('fs-toggle');
  if (!cb) return;
  cb.addEventListener('change', async () => {
    try {
      if (cb.checked) {
        const el = document.documentElement;
        if (el.requestFullscreen) await el.requestFullscreen();
        else if (el.webkitRequestFullscreen) await el.webkitRequestFullscreen();
      } else {
        if (document.exitFullscreen) await document.exitFullscreen();
        else if (document.webkitExitFullscreen) await document.webkitExitFullscreen();
      }
    } catch (e) { cb.checked = false; }
  });
  document.addEventListener('fullscreenchange', () => {
    cb.checked = !!(document.fullscreenElement || document.webkitFullscreenElement);
  });
}

function initPwa() {
  if ('serviceWorker' in navigator) {
    navigator.serviceWorker.register('/sw.js').catch(() => {});
  }
}

async function fetchSensors() {
  try {
    const s = await API.get('/api/sensors');
    setOnline(true);

    const temp = fmt(s.temperature, 1);
    const hum = fmt(s.humidity, 1);
    const pressure = fmt(s.pressure, 1);
    $('value-temp').textContent = temp;
    $('value-hum').textContent = hum;
    $('value-pressure').textContent = pressure;
    $('value-bmp-temp').textContent = fmt(s.bmpTemperature, 1);
    $('value-age').textContent = Number.isFinite(s.ageMs) ? (s.ageMs / 1000).toFixed(1) + ' s' : '--';

    $('ticker-text').textContent = 'T ' + temp + '°C  |  UR ' + hum + '%  |  P ' + pressure + ' hPa';

    setTag('tag-aht', s.aht === true, ['AHT20 ok', 'AHT20 ausente']);
    setTag('tag-aht-hum', s.aht === true, ['AHT20 ok', 'AHT20 ausente']);
    setTag('tag-bmp', s.bmp === true, ['BMP280 ok', 'BMP280 ausente']);

    renderAlerts(s.alerts || {});
    $('value-trend30m').textContent = signed(s.alerts && s.alerts.trend30m, 2) + ' hPa';
    $('value-trend3h').textContent = signed(s.alerts && s.alerts.trend3h, 2) + ' hPa';
    $('value-temprise').textContent = signed(s.alerts && s.alerts.tempRise, 2) + ' °C';
    $('trend-note').textContent = (s.alerts && s.alerts.trendReady) ? 'Tendência disponível' : 'Coletando histórico de pressão...';
    $('value-uptime').textContent = uptimeText(s.uptime);
  } catch (error) {
    setOnline(false);
  }
}

async function loadConfig() {
  try {
    const cfg = await API.get('/api/config');
    $('cfg-interval').value = cfg.readIntervalMs;
    $('cfg-hot').value = cfg.hotC;
    $('cfg-cold').value = cfg.coldC;
    $('cfg-dry').value = cfg.dryHumidityPct;
    $('cfg-rain').value = cfg.rainHumidityPct;
    $('value-interval').textContent = (cfg.readIntervalMs / 1000).toFixed(1) + ' s';
    setOnline(true);
  } catch (error) {
    setOnline(false);
  }
}

async function loadWifi() {
  try {
    const wifi = await API.get('/api/wifi');
    $('cfg-wifi-mode').value = wifi.mode || 'ap';
    $('cfg-wifi-ssid').value = wifi.staSsid || '';
    $('cfg-ap-ssid').value = wifi.apSsid || 'MSA-Setup';
    applyWifiMode();
    setOnline(true);
  } catch (error) {
    setOnline(false);
  }
}

async function fetchSystemInfo() {
  try {
    const info = await API.get('/api/system/info');
    $('cfg-system-info').textContent = JSON.stringify(info, null, 2);
    $('value-heap').textContent = (info.heap / 1024).toFixed(1) + ' KB';
    $('value-display').textContent = info.display ? (info.displayAddr || 'ok') : 'ausente';
    setOnline(true);
  } catch (error) {
    setOnline(false);
  }
}

async function saveLimits() {
  try {
    await API.post('/api/config', {
      readIntervalMs: Number($('cfg-interval').value),
      hotC: Number($('cfg-hot').value),
      coldC: Number($('cfg-cold').value),
      dryHumidityPct: Number($('cfg-dry').value),
      rainHumidityPct: Number($('cfg-rain').value)
    });
    setStatus('Limites salvos.', false);
    loadConfig();
    setOnline(true);
  } catch (error) {
    setStatus('Erro ao salvar limites: ' + error.message, true);
    setOnline(false);
  }
}

async function saveWifi() {
  try {
    const mode = $('cfg-wifi-mode').value;
    const payload = mode === 'sta'
      ? { mode, ssid: $('cfg-wifi-ssid').value.trim(), pass: $('cfg-wifi-pass').value }
      : { mode, ssid: $('cfg-ap-ssid').value.trim(), pass: $('cfg-ap-pass').value };
    await API.post('/api/wifi', payload);
    setStatus('Wi-Fi salvo. Reiniciando...', false);
    setOnline(true);
  } catch (error) {
    setStatus('Erro ao salvar Wi-Fi: ' + error.message, true);
    setOnline(false);
  }
}

async function restart() {
  try {
    await API.post('/api/system/restart', {});
    setStatus('Reiniciando...', false);
  } catch (error) {
    setStatus('Erro ao reiniciar: ' + error.message, true);
  }
}

function init() {
  initTabs();
  initWifiMode();
  initFullscreen();
  initPwa();

  $('cfg-save-limits').addEventListener('click', saveLimits);
  $('cfg-save-wifi').addEventListener('click', saveWifi);
  $('cfg-sys-info').addEventListener('click', fetchSystemInfo);
  $('cfg-restart').addEventListener('click', restart);

  fetchSensors();
  loadConfig();
  loadWifi();
  fetchSystemInfo();
  setInterval(fetchSensors, 2000);
}

init();
