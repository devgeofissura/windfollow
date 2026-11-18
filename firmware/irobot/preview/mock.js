(function () {
  'use strict';

  const params = new URLSearchParams(window.location.search);
  const scenario = {
    alerts: params.has('alerts'),
    fire: params.has('fire'),
    stuck: params.has('stuck')
  };

  const mock = {
    '/api/sensors': {
      temperature: scenario.fire ? 52.7 : 28.4,
      humidity: scenario.alerts ? 34 : 62,
      pressure: scenario.alerts ? 999.4 : 1008.2,
      distanceMm: 420,
      aht: true, bmp: true, tof: true,
      auto: false,
      autoState: 'OFF',
      avoidCount: 0,
      stuck: scenario.stuck,
      stuckSeq: scenario.stuck ? 1 : 0,
      alerts: {
        fire: scenario.fire,
        rainSensor: false,
        rain: scenario.alerts,
        hot: scenario.alerts,
        cold: false,
        dry: scenario.alerts,
        trendReady: true,
        trend30m: scenario.alerts ? -1.8 : -0.4,
        trend3h: scenario.alerts ? -3.2 : -1.1
      }
    },
    '/api/motor/status': {
      direction: 0,
      leftSpeed: 0,
      rightSpeed: 0,
      moving: false
    },
    '/api/config': {
      pwmMin: 60,
      pwmMax: 255,
      pwmDefault: 150,
      motorsEnabled: true
    },
    '/api/system/info': {
      preview: true,
      chip: 'ESP32',
      mac: 'aa:bb:cc:dd:ee:ff',
      flashBytes: 4194304,
      freeHeap: 112344,
      uptimeSec: 3600
    },
    '/api/wifi': { ok: true, mode: 'ap' }
  };

  function respond(body, status) {
    return {
      ok: status >= 200 && status < 300,
      status: status || 200,
      headers: { get: (k) => (String(k).toLowerCase() === 'content-type' ? 'application/json' : null) },
      json: async () => body,
      text: async () => JSON.stringify(body)
    };
  }

  const originalFetch = window.fetch.bind(window);
  window.fetch = function (input, opts) {
    const url = String(input);
    const path = url.split('?')[0];
    if (Object.prototype.hasOwnProperty.call(mock, path)) {
      const data = mock[path];
      if (opts && opts.method && opts.method.toUpperCase() === 'POST' && path !== '/api/system/info') {
        if (path === '/api/config' && opts.body) Object.assign(mock['/api/config'], JSON.parse(opts.body));
        if (path === '/api/wifi' && opts.body) Object.assign(mock['/api/wifi'], JSON.parse(opts.body));
        if (path === '/api/motor/move' && opts.body) {
          const b = JSON.parse(opts.body);
          mock['/api/motor/status'] = {
            direction: b.direction ? 1 : 0,
            leftSpeed: b.leftSpeed || b.speed || 0,
            rightSpeed: b.rightSpeed || b.speed || 0,
            moving: true
          };
        }
        if (path === '/api/motor/stop') {
          mock['/api/motor/status'] = { direction: 0, leftSpeed: 0, rightSpeed: 0, moving: false };
        }
        return respond({ ok: true }, 200);
      }
      return respond(data, 200);
    }
    return originalFetch(input, opts);
  };

  if ('serviceWorker' in navigator) {
    navigator.serviceWorker.getRegistrations().then(regs => regs.forEach(r => r.unregister())).catch(() => {});
  }
})();
