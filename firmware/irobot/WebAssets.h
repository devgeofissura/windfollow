#ifndef WEB_ASSETS_H
#define WEB_ASSETS_H

// Conteudos gerados automaticamente a partir da pasta web/
// Nao edite manualmente - rode generate_web_assets.ps1 apos alterar os arquivos web/
static const char* index_html = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-BR">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no">
<title>iRobot</title>
<link rel="stylesheet" href="/style.css?v=2">
</head>
<body>
<h1>iRobot</h1>

<div class="pad">
  <button class="btn" data-dir="frente">&#9650;<span>Frente</span></button>
  <button class="btn" data-dir="esquerda">&#9664;<span>Esquerda</span></button>
  <button class="btn stop" data-dir="stop">&#9632;<span>Parar</span></button>
  <button class="btn" data-dir="direita">&#9654;<span>Direita</span></button>
  <button class="btn" data-dir="tras">&#9660;<span>Tr&#225;s</span></button>
</div>

<p id="status">toque em um bot&#227;o</p>

<script src="/app.js?v=2"></script>
</body>
</html>

)rawliteral";
static const char* style_css = R"rawliteral(
* { box-sizing: border-box; }

html, body {
  margin: 0;
  height: 100%;
  background: #101418;
  color: #e8eef5;
  font-family: system-ui, -apple-system, "Segoe UI", Roboto, sans-serif;
  -webkit-user-select: none;
  user-select: none;
  touch-action: manipulation;
}

body {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  gap: 16px;
  padding: 16px;
}

h1 {
  margin: 0;
  font-size: 1.4rem;
  font-weight: 600;
  letter-spacing: 0.04em;
  color: #8fd7ff;
}

.pad {
  display: grid;
  grid-template-columns: repeat(3, minmax(88px, 120px));
  grid-template-rows: repeat(3, minmax(88px, 120px));
  gap: 10px;
}

.btn {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  gap: 6px;
  border: 0;
  border-radius: 16px;
  background: #1d2733;
  color: #e8eef5;
  font-size: 1.6rem;
  font-family: inherit;
  cursor: pointer;
  -webkit-tap-highlight-color: transparent;
  transition: background 0.1s, transform 0.05s;
}

.btn span {
  font-size: 0.85rem;
  color: #9fb2c6;
}

.btn:active,
.btn.active {
  background: #2f9fd8;
  transform: scale(0.97);
}

.btn:active span,
.btn.active span {
  color: #eaf7ff;
}

.stop {
  background: #3a1f24;
}

.stop span {
  color: #ffb3b3;
}

.stop:active,
.stop.active {
  background: #d64553;
}

.btn[data-dir="frente"] { grid-area: 1 / 2; }
.btn[data-dir="esquerda"] { grid-area: 2 / 1; }
.btn[data-dir="stop"] { grid-area: 2 / 2; }
.btn[data-dir="direita"] { grid-area: 2 / 3; }
.btn[data-dir="tras"] { grid-area: 3 / 2; }

#status {
  margin: 0;
  min-height: 1.4em;
  font-size: 0.95rem;
  color: #9fb2c6;
  text-align: center;
}

#status.error {
  color: #ff8a8a;
}

)rawliteral";
static const char* app_js = R"rawliteral(
(function () {
  'use strict';

  var statusEl = document.getElementById('status');
  var buttons = Array.prototype.slice.call(document.querySelectorAll('.btn'));
  var busy = false;

  function setStatus(text, isError) {
    statusEl.textContent = text;
    statusEl.className = isError ? 'error' : '';
  }

  function send(dir) {
    var path = dir === 'stop' ? '/api/motor/stop' : '/api/motor/move';
    var payload = dir === 'stop' ? {} : { direction: dir };

    return fetch(path, {
      method: 'POST',
      cache: 'no-store',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(payload)
    }).then(function (res) {
      return res.text().then(function (text) {
        if (!res.ok) throw new Error('HTTP ' + res.status + ' ' + text);
        return text;
      });
    });
  }

  function markActive(dir) {
    buttons.forEach(function (b) {
      b.classList.toggle('active', b.getAttribute('data-dir') === dir);
    });
  }

  buttons.forEach(function (btn) {
    var dir = btn.getAttribute('data-dir');

    function trigger(event) {
      event.preventDefault();
      if (busy) return;
      busy = true;
      markActive(dir);
      setStatus('enviando ' + dir + '...');
      send(dir).then(function () {
        setStatus(dir === 'stop' ? 'parado' : 'movendo: ' + dir);
      }).catch(function (err) {
        markActive(null);
        setStatus('falha: ' + err.message, true);
      }).then(function () {
        busy = false;
      });
    }

    btn.addEventListener('click', trigger);
  });

  setStatus('toque em um bot\u00e3o');
})();

)rawliteral";

#endif // WEB_ASSETS_H