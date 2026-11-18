const API = {
  async request(path, options) {
    const response = await fetch(path, options);
    if (!response.ok) throw new Error((await response.text()) || ('HTTP ' + response.status));
    return response.json();
  },
  get(path) { return this.request(path); },
  post(path, body) {
    return this.request(path, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(body)
    });
  }
};

const byId = (id) => document.getElementById(id);
const fmt = (value, digits) => (Number.isFinite(value) ? value.toFixed(digits) : '--');
const signed = (value, digits) => (Number.isFinite(value) ? (value >= 0 ? '+' : '') + value.toFixed(digits) : '--');

function setTag(id, ok, labels) {
  const el = byId(id);
  el.textContent = ok ? labels[0] : labels[1];
  el.className = 'sensor-tag ' + (ok ? 'ok' : 'fail');
}

function uptimeText(ms) {
  if (!Number.isFinite(ms)) return '--';
  const total = Math.floor(ms / 1000);
  const h = Math.floor(total / 3600);
  const m = Math.floor((total % 3600) / 60);
  const s = total % 60;
  return (h ? h + 'h ' : '') + m + 'm ' + s + 's';
}

function renderAlerts(alerts) {
  const box = byId('alerts');
  const items = [];
  if (alerts.fire) items.push(['INCÊNDIO', 'crit']);
  if (alerts.rain) items.push(['Chuva provável', 'warn']);
  if (alerts.hot) items.push(['Calor', 'warn']);
  if (alerts.cold) items.push(['Frio', 'info']);
  if (alerts.dry) items.push(['Ar seco', 'warn']);
  if (!items.length) items.push(['Sem avisos', 'ok']);
  box.innerHTML = items.map(([text, kind]) => '<span class="chip ' + kind + '">' + text + '</span>').join('');
}

async function refreshSensors() {
  try {
    const s = await API.get('/api/sensors');
    byId('badge-conn').textContent = 'Online';
    byId('badge-conn').className = 'badge online';

    byId('value-temp').textContent = fmt(s.temperature, 1);
    byId('value-hum').textContent = fmt(s.humidity, 1);
    byId('value-pressure').textContent = fmt(s.pressure, 1);
    byId('value-bmp-temp').textContent = fmt(s.bmpTemperature, 1);
    byId('value-age').textContent = Number.isFinite(s.ageMs) ? (s.ageMs / 1000).toFixed(1) + ' s' : '--';

    setTag('tag-aht', s.aht === true, ['AHT20 ok', 'AHT20 ausente']);
    setTag('tag-aht-hum', s.aht === true, ['AHT20 ok', 'AHT20 ausente']);
    setTag('tag-bmp', s.bmp === true, ['BMP280 ok', 'BMP280 ausente']);

    renderAlerts(s.alerts || {});
    byId('value-trend30m').textContent = signed(s.alerts && s.alerts.trend30m, 2) + ' hPa';
    byId('value-trend3h').textContent = signed(s.alerts && s.alerts.trend3h, 2) + ' hPa';
    byId('value-temprise').textContent = signed(s.alerts && s.alerts.tempRise, 2) + ' °C';
    byId('trend-note').textContent = (s.alerts && s.alerts.trendReady)
      ? 'Tendência disponível'
      : 'Coletando histórico de pressão...';

    byId('value-uptime').textContent = uptimeText(s.uptime);
    byId('badge-uptime').textContent = uptimeText(s.uptime);

    const info = await API.get('/api/system/info');
    byId('value-heap').textContent = (info.heap / 1024).toFixed(1) + ' KB';
  } catch (error) {
    byId('badge-conn').textContent = 'Desconectado';
    byId('badge-conn').className = 'badge offline';
  }
}

async function loadConfig() {
  try {
    const cfg = await API.get('/api/config');
    byId('cfg-interval').value = cfg.readIntervalMs;
    byId('cfg-hot').value = cfg.hotC;
    byId('cfg-cold').value = cfg.coldC;
    byId('cfg-dry').value = cfg.dryHumidityPct;
    byId('cfg-rain').value = cfg.rainHumidityPct;
    byId('value-interval').textContent = (cfg.readIntervalMs / 1000).toFixed(1) + ' s';
  } catch (error) {
    setStatus(error.message, true);
  }
}

async function loadWifi() {
  try {
    const wifi = await API.get('/api/wifi');
    byId('wifi-mode').value = wifi.mode;
    byId('wifi-ssid').value = wifi.ssid || '';
  } catch (error) {
    setStatus(error.message, true);
  }
}

function setStatus(message, isError) {
  const el = byId('form-status');
  el.textContent = message;
  el.style.color = isError ? '#ffb3b3' : '#9fe3b2';
}

document.querySelectorAll('.tab').forEach((tab) => {
  tab.addEventListener('click', () => {
    document.querySelectorAll('.tab').forEach((t) => {
      t.classList.remove('active');
      t.setAttribute('aria-selected', 'false');
    });
    document.querySelectorAll('.panel').forEach((p) => p.classList.remove('active'));
    tab.classList.add('active');
    tab.setAttribute('aria-selected', 'true');
    byId('panel-' + tab.dataset.tab).classList.add('active');
    if (tab.dataset.tab === 'config') {
      loadConfig();
      loadWifi();
    }
  });
});

byId('form-config').addEventListener('submit', async (event) => {
  event.preventDefault();
  const payload = {
    readIntervalMs: Number(byId('cfg-interval').value),
    hotC: Number(byId('cfg-hot').value),
    coldC: Number(byId('cfg-cold').value),
    dryHumidityPct: Number(byId('cfg-dry').value),
    rainHumidityPct: Number(byId('cfg-rain').value)
  };
  try {
    await API.post('/api/config', payload);
    setStatus('Configuração salva.', false);
    loadConfig();
  } catch (error) {
    setStatus('Erro ao salvar: ' + error.message, true);
  }
});

byId('form-wifi').addEventListener('submit', async (event) => {
  event.preventDefault();
  const payload = {
    mode: byId('wifi-mode').value,
    ssid: byId('wifi-ssid').value.trim(),
    pass: byId('wifi-pass').value
  };
  try {
    await API.post('/api/wifi', payload);
    setStatus('Wi-Fi salvo. O dispositivo vai reiniciar.', false);
  } catch (error) {
    setStatus('Erro ao salvar Wi-Fi: ' + error.message, true);
  }
});

byId('btn-restart').addEventListener('click', async () => {
  if (!confirm('Reiniciar o MSA?')) return;
  try {
    await API.post('/api/system/restart', {});
    setStatus('Reiniciando...', false);
  } catch (error) {
    setStatus('Erro ao reiniciar: ' + error.message, true);
  }
});

refreshSensors();
loadConfig();
setInterval(refreshSensors, 2000);
