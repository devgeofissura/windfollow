#ifndef WEB_ASSETS_H
#define WEB_ASSETS_H

// Conteudos gerados automaticamente a partir da pasta web/
// Nao edite manualmente - rode generate_web_assets.ps1 apos alterar os arquivos web/
static const char* index_html = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-BR">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
  <meta name="theme-color" content="#0b1120">
  <meta name="apple-mobile-web-app-capable" content="yes">
  <meta name="apple-mobile-web-app-status-bar-style" content="black-translucent">
  <meta name="apple-mobile-web-app-title" content="iRobot">
  <link rel="manifest" href="/manifest.json">
  <link rel="icon" type="image/svg+xml" href="/favicon.svg">
  <link rel="apple-touch-icon" type="image/svg+xml" href="/favicon.svg">
  <title>iRobot</title>
  <link rel="stylesheet" href="/style.css?v=6">
</head>
<body>
  <div class="app">
    <!-- Letreiro de ambiente (topo, pequeno) -->
    <div class="ticker" id="ticker">
      <span class="ticker-label">Ambiente:</span>
      <span class="ticker-text" id="ticker-text">Iniciando sensores...</span>
    </div>

    <!-- Header com abas -->
    <header class="app-header">
      <div class="logo" id="app-logo">&#129302;</div>
      <h1 class="app-title" id="app-title">iRobot</h1>
      <div class="header-actions">
        <div id="badge-conn" class="badge offline">Desconectado</div>
        <label class="switch fullscreen-switch">
          <input type="checkbox" id="fs-toggle">
          <span class="slider-round"></span>
        </label>
        <span class="fs-hint" id="fs-hint">Tela cheia</span>
      </div>
    </header>

    <nav class="tabs" role="tablist">
      <button class="tab active" role="tab" aria-selected="true" data-tab="home">Home</button>
      <button class="tab" role="tab" aria-selected="false" data-tab="dl">DL</button>
      <button class="tab" role="tab" aria-selected="false" data-tab="config">Config</button>
    </nav>

    <!-- =================== HOME =================== -->
    <section class="panel panel-home" id="panel-home">
      <div class="home-grid">
        <!-- Coluna esquerda: joystick -->
        <aside class="home-col joy-col">
          <div class="card joy-card">
            <div class="card-title">Joystick</div>
            <div class="joystick-wrap">
              <div class="joystick-base" id="joy-base">
                <div class="joystick-center-dot"></div>
                <div class="joystick-knob" id="joy-knob"></div>
              </div>
              <div class="joy-readout">
                <span id="joy-dir">Parado</span>
                <span id="joy-int">Int: 0%</span>
              </div>
            </div>
          </div>
        </aside>

        <!-- Coluna central: painel de botoes -->
        <section class="home-col center-col">
          <div class="card btns-card">
            <div class="card-title">Comandos</div>
            <div class="dpad">
              <button class="btn dpad-btn" id="btn-frente" data-dir="frente">&#9650;<span>Frente</span></button>
              <div class="dpad-row">
                <button class="btn dpad-btn" id="btn-esquerda" data-dir="esquerda">&#9664;<span>Esquerda</span></button>
                <button class="btn dpad-btn btn-stop" id="btn-stop">&#9632;<span>Parar</span></button>
                <button class="btn dpad-btn" id="btn-direita" data-dir="direita">&#9654;<span>Direita</span></button>
              </div>
              <button class="btn dpad-btn" id="btn-tras" data-dir="tras">&#9660;<span>Tras</span></button>
            </div>
            <button class="btn btn-auto btn-block" id="btn-home-auto">Modo Autonomo: OFF</button>
            <div class="speed-row">
              <label class="speed-label">Velocidade</label>
              <input type="range" id="home-speed" min="0" max="100" value="70" class="hslider">
              <span id="home-speed-val" class="speed-val">70%</span>
            </div>
          </div>
        </section>

        <!-- Coluna direita: sensores + status -->
        <aside class="home-col info-col">
          <div class="card info-card">
            <div class="card-title">Sensores</div>
            <div class="env-grid">
              <div class="env-item"><span class="env-k">Temp</span><strong id="home-temp" class="env-v">--</strong></div>
              <div class="env-item"><span class="env-k">Umidade</span><strong id="home-hum" class="env-v">--</strong></div>
              <div class="env-item"><span class="env-k">Pressao</span><strong id="home-pres" class="env-v">--</strong></div>
              <div class="env-item"><span class="env-k">Distancia</span><strong id="home-dist" class="env-v">--</strong></div>
            </div>
          </div>
          <div class="card status-card">
            <div class="card-title">Status do Robo</div>
            <div class="status-row">
              <span class="status-k">Estado:</span>
              <strong id="home-state" class="status-v">Parado</strong>
            </div>
            <div class="status-row">
              <span class="status-k">Motores:</span>
              <strong id="home-moving" class="status-v">Parado</strong>
            </div>
            <div class="status-row">
              <span class="status-k">Conexao:</span>
              <strong id="home-conn" class="status-v offline">Desconectado</strong>
            </div>
          </div>
        </aside>
      </div>
    </section>

    <!-- =================== DL (antiga tela de teste) =================== -->
    <section class="panel panel-dl hidden" id="panel-dl">
      <div class="dl-wrap">
        <section class="dl-col-left">
          <div class="card joy-card">
            <div class="card-title">Joystick Diferencial</div>
            <div class="joystick-wrap">
              <div class="joystick-base" id="dl-joy-base">
                <div class="joystick-center-dot"></div>
                <div class="joystick-knob" id="dl-joy-knob"></div>
              </div>
              <div class="joy-readout">
                <span id="dl-joy-dir">Parado</span>
                <span id="dl-joy-int">Int: 0%</span>
              </div>
            </div>
          </div>
        </section>

        <section class="dl-col-right">
          <div class="card cmds-card">
            <div class="card-title">Comandos Rapidos</div>
            <div class="dpad">
              <button class="btn dpad-btn" id="dl-frente" data-dir="frente">Frente</button>
              <div class="dpad-row">
                <button class="btn dpad-btn" id="dl-esquerda" data-dir="esquerda">Esquerda</button>
                <button class="btn dpad-btn btn-stop" id="dl-stop">Parar</button>
                <button class="btn dpad-btn" id="dl-direita" data-dir="direita">Direita</button>
              </div>
              <button class="btn dpad-btn" id="dl-tras" data-dir="tras">Tras</button>
            </div>
          </div>

          <div class="card speed-card">
            <div class="card-title">Velocidade</div>
            <div class="slider-row">
              <label class="slider-label">Geral</label>
              <input type="range" id="dl-speed" min="0" max="100" value="70" class="hslider">
              <span id="dl-speed-val" class="speed-val">70%</span>
            </div>
          </div>

          <div class="card diff-card">
            <div class="card-title">Motores Independentes</div>
            <div class="slider-row">
              <label class="slider-label">Esquerda</label>
              <input type="range" id="dl-left" min="-100" max="100" value="0" class="hslider">
              <span id="dl-left-val" class="speed-val">0</span>
            </div>
            <div class="slider-row">
              <label class="slider-label">Direita</label>
              <input type="range" id="dl-right" min="-100" max="100" value="0" class="hslider">
              <span id="dl-right-val" class="speed-val">0</span>
            </div>
            <button class="btn btn-outline btn-block" id="dl-apply-diff">Aplicar Diferencial</button>
          </div>

          <div class="card pwm-card">
            <div class="card-title">Configuracao PWM</div>
            <div class="config-row"><label>Min</label><input type="number" id="dl-pwm-min" min="0" max="255" value="60" class="cfg-input"></div>
            <div class="config-row"><label>Max</label><input type="number" id="dl-pwm-max" min="0" max="255" value="255" class="cfg-input"></div>
            <div class="config-row"><label>Default</label><input type="number" id="dl-pwm-default" min="0" max="255" value="150" class="cfg-input"></div>
            <div class="config-row"><label>Motores</label><label class="switch inline"><input type="checkbox" id="dl-motors-enabled" checked><span class="slider-round"></span></label></div>
            <button class="btn btn-outline btn-block" id="dl-save-config">Salvar Configuracao</button>
          </div>

          <div class="card env-card">
            <div class="card-title">Ambiente</div>
            <div class="env-grid">
              <div class="env-item"><span class="env-k">Temp</span><strong id="dl-temp" class="env-v">--</strong></div>
              <div class="env-item"><span class="env-k">Umidade</span><strong id="dl-hum" class="env-v">--</strong></div>
              <div class="env-item"><span class="env-k">Pressao</span><strong id="dl-pres" class="env-v">--</strong></div>
              <div class="env-item"><span class="env-k">Distancia</span><strong id="dl-dist" class="env-v">--</strong></div>
            </div>
            <div class="env-status" id="dl-env-status">sensores: verificando...</div>
            <button class="btn btn-auto btn-block" id="dl-auto">Autonomo: OFF</button>
          </div>

          <div class="card advice-card">
            <div class="card-title">Avisos</div>
            <ul class="advice-list" id="dl-advice-list"><li class="advice-none">Nenhum aviso no momento</li></ul>
            <div class="env-status" id="dl-trend-status">tendencia: coletando dados...</div>
          </div>

          <div class="card sys-card">
            <div class="card-title">Sistema</div>
            <button class="btn btn-outline btn-block" id="dl-sys-info">Ler Status do Sistema</button>
            <div id="dl-system-info" class="sys-info"></div>
          </div>
        </section>
      </div>

      <div class="telemetry" id="dl-telemetry">
        <div class="tel-item"><span class="tel-label">Status:</span><span id="dl-tel-status" class="tel-value">Parado</span></div>
        <div class="tel-item"><span class="tel-label">Motor Esq:</span><span id="dl-tel-left" class="tel-value">0</span></div>
        <div class="tel-item"><span class="tel-label">Motor Dir:</span><span id="dl-tel-right" class="tel-value">0</span></div>
        <div class="tel-item"><span class="tel-label">Movendo:</span><span id="dl-tel-moving" class="tel-value">Nao</span></div>
        <div class="tel-item"><span class="tel-label">Conexao:</span><span id="dl-tel-conn" class="tel-value">Offline</span></div>
        <div class="tel-item"><span class="tel-label">Ultimo erro:</span><span id="dl-tel-api-error" class="tel-value offline">Nenhum</span></div>
      </div>

      <div class="alert alert-critical" id="dl-alert-fire" hidden>
        <strong>Possivel incendio</strong>
        <p id="dl-alert-fire-text">A temperatura subiu muito rapido. Verifique o ambiente imediatamente.</p>
        <button id="dl-btn-fire-ok" type="button">Entendi</button>
      </div>
      <div class="alert" id="dl-alert-stuck" hidden>
        <strong>Robo enroscado</strong>
        <p id="dl-alert-stuck-text">O modo autonomo foi desligado apos 100 obstaculos seguidos. Limpe o caminho e ligue novamente.</p>
        <button id="dl-btn-alert-ok" type="button">Entendi</button>
      </div>
    </section>

    <!-- =================== CONFIG =================== -->
    <section class="panel panel-config hidden" id="panel-config">
      <div class="config-wrap">
        <div class="card wifi-card">
          <div class="card-title">Wi-Fi</div>
          <div class="config-row">
            <label>Modo</label>
            <select id="cfg-wifi-mode" class="cfg-select">
              <option value="ap">Access Point (server)</option>
              <option value="sta">Cliente (client)</option>
            </select>
          </div>
          <div class="config-row sta-row" id="cfg-wifi-sta-row" style="display:none;">
            <label>SSID</label><input type="text" id="cfg-wifi-ssid" class="cfg-input" placeholder="Nome da rede">
          </div>
          <div class="config-row sta-row" id="cfg-wifi-pass-row" style="display:none;">
            <label>Senha</label><input type="password" id="cfg-wifi-pass" class="cfg-input" placeholder="Senha Wi-Fi">
          </div>
          <div class="config-row ap-row" id="cfg-wifi-ap-row">
            <label>SSID AP</label><input type="text" id="cfg-ap-ssid" class="cfg-input" value="Irobot-Setup">
          </div>
          <div class="config-row ap-row" id="cfg-wifi-ap-pass-row">
            <label>Senha AP</label><input type="password" id="cfg-ap-pass" class="cfg-input" value="irobot1234">
          </div>
          <button class="btn btn-outline btn-block" id="cfg-save-wifi">Salvar Wi-Fi</button>
        </div>

        <div class="card pwm-card">
          <div class="card-title">PWM / Motores</div>
          <div class="config-row"><label>Min</label><input type="number" id="cfg-pwm-min" min="0" max="255" value="60" class="cfg-input"></div>
          <div class="config-row"><label>Max</label><input type="number" id="cfg-pwm-max" min="0" max="255" value="255" class="cfg-input"></div>
          <div class="config-row"><label>Default</label><input type="number" id="cfg-pwm-default" min="0" max="255" value="150" class="cfg-input"></div>
          <div class="config-row"><label>Motores habilitados</label><label class="switch inline"><input type="checkbox" id="cfg-motors-enabled" checked><span class="slider-round"></span></label></div>
          <button class="btn btn-outline btn-block" id="cfg-save-pwm">Salvar PWM</button>
        </div>

        <div class="card sys-card">
          <div class="card-title">Sistema</div>
          <button class="btn btn-outline btn-block" id="cfg-sys-info">Ler Status do Sistema</button>
          <div id="cfg-system-info" class="sys-info"></div>
          <div class="config-row" style="margin-top:12px;">
            <button class="btn btn-outline btn-block" id="cfg-restart">Reiniciar Robo</button>
          </div>
        </div>
      </div>
    </section>
  </div>

  <div class="alert alert-critical" id="alert-fire" hidden>
    <strong>Possivel incendio</strong>
    <p id="alert-fire-text">A temperatura subiu muito rapido. Verifique o ambiente imediatamente.</p>
    <button id="btn-fire-ok" type="button">Entendi</button>
  </div>
  <div class="alert" id="alert-stuck" hidden>
    <strong>Robo enroscado</strong>
    <p id="alert-stuck-text">O modo autonomo foi desligado apos 100 obstaculos seguidos. Limpe o caminho e ligue novamente.</p>
    <button id="btn-alert-ok" type="button">Entendi</button>
  </div>

  <script src="/app.js?v=6"></script>
</body>
</html>

)rawliteral";
static const char* style_css = R"rawliteral(
* { box-sizing: border-box; }
html, body {
  margin: 0; padding: 0;
  height: 100%; width: 100%;
  background: #0b1120; color: #e8eef5;
  font-family: system-ui, -apple-system, "Segoe UI", Roboto, sans-serif;
  -webkit-user-select: none; user-select: none; touch-action: manipulation;
  overflow: hidden;
}
body { display: flex; flex-direction: column; }

/* Letreiro de ambiente */
.ticker {
  width: 100%;
  background: #121a2b;
  color: #cfe3f5;
  font-size: 0.82rem;
  padding: 4px 10px;
  overflow: hidden;
  white-space: nowrap;
  border-bottom: 1px solid #1f2a3d;
  z-index: 40;
}
.ticker-label { color: #8fd7ff; margin-right: 8px; font-weight: 600; }
.ticker-text { display: inline-block; animation: ticker 20s linear infinite; }
@keyframes ticker { 0% { transform: translateX(100%); } 100% { transform: translateX(-100%); } }

/* Header */
.app-header {
  display: flex; align-items: center; justify-content: space-between;
  height: 56px; padding: 0 14px;
  background: #0f172a; border-bottom: 1px solid #1f2a3d;
  z-index: 30;
}
.logo { font-size: 1.6rem; width: 32px; height: 32px; display: flex; align-items: center; justify-content: center; border-radius: 8px; background: #1d2733; }
.app-title { margin: 0 0 0 10px; font-size: 1.15rem; font-weight: 600; letter-spacing: 0.02em; color: #8fd7ff; }
.header-actions { display: flex; align-items: center; gap: 10px; }
.badge {
  padding: 4px 10px; border-radius: 12px; font-size: 0.78rem; font-weight: 600;
  background: #1f2a3d; color: #9fb2c6; border: 1px solid #2d3a50;
}
.badge.online { background: #163a26; color: #9fe3b2; border-color: #2fa76a; }
.badge.offline { background: #3a2428; color: #ffb3b3; border-color: #d64553; }

/* Tela cheia switch */
.fullscreen-switch { position: relative; display: inline-block; width: 44px; height: 24px; }
.fullscreen-switch input { opacity: 0; width: 0; height: 0; }
.slider-round {
  position: absolute; cursor: pointer; inset: 0; background: #1f2a3d;
  border: 1px solid #2d3a50; border-radius: 24px; transition: .2s;
}
.slider-round:before { position: absolute; content: ""; height: 18px; width: 18px; left: 3px; bottom: 3px; background: #fff; border-radius: 50%; transition: .2s; }
.fullscreen-switch input:checked + .slider-round { background: #2f9fd8; border-color: #2f9fd8; }
.fullscreen-switch input:checked + .slider-round:before { transform: translateX(20px); }
.fs-hint { font-size: 0.72rem; color: #8fd7ff; display: none; }
.fullscreen-switch input:checked ~ .fs-hint { display: inline; }

/* Tabs */
.tabs {
  display: flex; background: #0f172a; border-bottom: 1px solid #1f2a3d; gap: 0;
}
.tab {
  flex: 1; padding: 10px 4px; border: 0; background: transparent; color: #8fd7ff;
  font-size: 0.9rem; font-weight: 600; cursor: pointer; border-bottom: 2px solid transparent;
}
.tab.active { color: #8fd7ff; border-bottom-color: #2f9fd8; background: #121a2b; }

/* Panels */
.app > main, .app { flex: 1; overflow: hidden; display: flex; flex-direction: column; }
.panel { display: none; flex: 1; overflow: hidden; flex-direction: column; }
.panel.active { display: flex; }

/* ---------- HOME (paisagem) ---------- */
.panel-home { position: relative; }
.home-grid {
  display: grid;
  grid-template-columns: 250px 1fr 260px;
  grid-template-rows: 1fr auto;
  gap: 10px; padding: 10px; height: calc(100% - 56px);
  overflow: hidden;
}
.home-col { display: flex; flex-direction: column; gap: 10px; }
.joy-col { grid-column: 1; grid-row: 1 / 3; }
.center-col { grid-column: 2; grid-row: 1 / 3; }
.info-col { grid-column: 3; grid-row: 1 / 3; }
.home-grid > .card { flex: 1; }

/* Quando modo retrato, empilha colunas */
@media (max-width: 720px) and (orientation: portrait) {
  .home-grid { grid-template-columns: 1fr; grid-template-rows: auto auto auto; }
  .joy-col, .center-col, .info-col { grid-column: 1; }
}

/* Cards */
.card {
  background: #121a2b; border: 1px solid #1f2a3d; border-radius: 14px;
  padding: 12px; display: flex; flex-direction: column; gap: 10px;
}
.card-title {
  margin: 0; font-size: 0.85rem; font-weight: 700; letter-spacing: 0.04em;
  color: #8fd7ff; border-bottom: 1px solid #1f2a3d; padding-bottom: 6px;
}

/* Joystick */
.joy-wrap { flex: 1; display: flex; flex-direction: column; align-items: center; gap: 8px; }
.joystick-base {
  position: relative; width: 150px; height: 150px; border-radius: 50%;
  background: #1d2733; border: 2px solid #2d3a50; touch-action: none;
}
.joystick-center-dot {
  position: absolute; left: 50%; top: 50%; width: 10px; height: 10px;
  background: #2d3a50; border-radius: 50%; transform: translate(-50%, -50%);
}
.joystick-knob {
  position: absolute; left: 50%; top: 50%; width: 44px; height: 44px;
  background: #2f9fd8; border-radius: 50%; transform: translate(-50%, -50%);
  border: 2px solid #121a2b; box-shadow: 0 4px 12px rgba(47,159,216,0.35);
  cursor: grab; touch-action: none;
}
.joy-readout { display: flex; gap: 12px; font-size: 0.8rem; color: #9fb2c6; }

/* D-pad */
.dpad {
  display: grid; grid-template-columns: repeat(3, 1fr); grid-template-rows: repeat(3, 1fr);
  gap: 10px; margin-top: 6px;
}
.btn {
  border: 0; border-radius: 12px; background: #1d2733; color: #e8eef5;
  font-size: 1rem; font-weight: 600; cursor: pointer; -webkit-tap-highlight-color: transparent;
  display: flex; flex-direction: column; align-items: center; justify-content: center; gap: 4px;
  transition: background .1s, transform .05s;
}
.btn span { font-size: 0.72rem; color: #9fb2c6; }
.btn:active, .btn.active { background: #2f9fd8; transform: scale(0.96); }
.btn:active span, .btn.active span { color: #eaf7ff; }
.btn-stop { background: #3a1f24; }
.btn-stop span { color: #ffb3b3; }
.btn-stop:active, .btn-stop.active { background: #d64553; }
.btn-outline { background: transparent; border: 1px solid #2d3a50; color: #8fd7ff; }
.btn-outline:active { background: #1f2a3d; }
.btn-block { width: 100%; flex-direction: row; padding: 10px 14px; justify-content: center; }
.btn-auto { background: #16202b; border: 1px solid #2f4257; color: #cfe3f5; }
.btn-auto.on { background: #1f7a4d; border-color: #2fa76a; color: #eafff3; }

/* Velocidade home */
.speed-row { display: flex; align-items: center; gap: 10px; margin-top: 6px; }
.speed-label { font-size: 0.8rem; color: #9fb2c6; width: 70px; }
.hslider { -webkit-appearance: none; width: 100%; height: 6px; border-radius: 3px; background: #1f2a3d; outline: 0; }
.hslider::-webkit-slider-thumb { -webkit-appearance: none; width: 16px; height: 16px; border-radius: 50%; background: #2f9fd8; cursor: pointer; }
.speed-val { font-size: 0.8rem; color: #8fd7ff; min-width: 36px; text-align: right; }

/* Sensores / info */
.env-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 6px 10px; margin-top: 4px; }
.env-item { display: flex; justify-content: space-between; font-size: 0.78rem; }
.env-k { color: #9fb2c6; }
.env-v { color: #cfe3f5; font-variant-numeric: tabular-nums; }
.status-row { display: flex; justify-content: space-between; font-size: 0.8rem; padding: 4px 0; border-bottom: 1px solid #161e2f; }
.status-row:last-child { border-bottom: 0; }
.status-k { color: #9fb2c6; }
.status-v { color: #cfe3f5; }

/* ---------- DL (antiga tela, mantida) ---------- */
.panel-dl { position: relative; }
.dl-wrap {
  display: grid; grid-template-columns: 260px 1fr; gap: 10px; padding: 10px;
  height: calc(100% - 56px); overflow: hidden;
}
.dl-col-left, .dl-col-right { display: flex; flex-direction: column; gap: 10px; }
.dl-col-left { grid-column: 1; }
.dl-col-right { grid-column: 2; }
.dl-card, .dl-card > .card { flex: 1; }
.telemetry {
  position: absolute; bottom: 0; left: 0; right: 0;
  background: #121a2b; border-top: 1px solid #1f2a3d;
  display: grid; grid-template-columns: repeat(auto-fit, minmax(120px, 1fr));
  gap: 6px; padding: 6px 10px; font-size: 0.72rem; z-index: 20;
}
.tel-item { display: flex; justify-content: space-between; gap: 8px; }
.tel-label { color: #9fb2c6; font-size: 0.72rem; }
.tel-value { color: #cfe3f5; font-variant-numeric: tabular-nums; }
.tel-value.online { color: #9fe3b2; }
.tel-value.offline { color: #ffb3b3; }

/* Alerts */
.alert {
  position: fixed; top: 60px; left: 50%; transform: translateX(-50%);
  padding: 12px 18px; border-radius: 10px; z-index: 100;
  background: #121a2b; border: 1px solid #1f2a3d; box-shadow: 0 8px 24px rgba(0,0,0,.4);
  max-width: 90%; font-size: 0.85rem;
}
.alert-critical { background: #2a1f22; border-color: #d64553; color: #ffb3b3; }
.alert button { margin-top: 8px; background: #2f9fd8; border: 0; border-radius: 8px; color: #fff; padding: 6px 14px; cursor: pointer; }
.advice-list { margin: 4px 0 0 0; padding-left: 16px; font-size: 0.78rem; color: #9fb2c6; }
.advice-none { color: #5c6b7c; }

/* Config */
.config-wrap { display: grid; grid-template-columns: 1fr 1fr; gap: 10px; padding: 10px; height: calc(100% - 56px); overflow-y: auto; }
.config-wrap > .card { height: fit-content; }
.config-row { display: flex; justify-content: space-between; align-items: center; gap: 10px; padding: 6px 0; border-bottom: 1px solid #161e2f; }
.config-row:last-child { border-bottom: 0; }
.config-row label { font-size: 0.8rem; color: #9fb2c6; }
.cfg-input, .cfg-select { background: #1d2733; border: 1px solid #2d3a50; color: #e8eef5; border-radius: 8px; padding: 6px 10px; font-size: 0.85rem; }
.sta-row { display: none; }

/* Switch inline */
.switch.inline { position: relative; display: inline-block; width: 44px; height: 24px; }
.switch.inline input { opacity: 0; width: 0; height: 0; }
.switch.inline .slider-round { position: absolute; inset: 0; background: #1f2a3d; border: 1px solid #2d3a50; border-radius: 24px; cursor: pointer; transition: .2s; }
.switch.inline .slider-round:before { position: absolute; content: ""; height: 18px; width: 18px; left: 3px; bottom: 3px; background: #fff; border-radius: 50%; transition: .2s; }
.switch.inline input:checked + .slider-round { background: #2f9fd8; border-color: #2f9fd8; }
.switch.inline input:checked + .slider-round:before { transform: translateX(20px); }

.hidden { display: none !important; }

/* Paisagem forcada na home quando instalado/fullscreen */
@media screen and (orientation: portrait) {
  .panel-home { orientation: landscape; }
}

/* Esconde barra de endereco visualmente quando PWA (não pode remover de verdade sem install) */
.app { display: flex; flex-direction: column; height: 100vh; height: -webkit-fill-available; }

)rawliteral";
static const char* app_js = R"rawliteral(
'use strict';

// -------------------- API --------------------
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

// -------------------- estado --------------------
const state = {
  powerOn: true,
  speedPct: 70,
  leftPct: 0,
  rightPct: 0,
  autoOn: false,
  lastStuckSeq: 0,
  lastFire: false,
  online: false,
  activeTab: 'home',
  joystick: { base: null, knob: null, cx: 0, cy: 0, radius: 0, knobRadius: 22, active: false, touchId: null, x: 0, y: 0, intensity: 0 },
  lastSent: { left: 0, right: 0, dir: '', speed: 0 }
};

const joyState = {
  home: { base: null, knob: null, cx: 0, cy: 0, radius: 0, dirEl: null, intEl: null, active: false, touchId: null, x: 0, y: 0, intensity: 0 },
  dl:   { base: null, knob: null, cx: 0, cy: 0, radius: 0, dirEl: null, intEl: null, active: false, touchId: null, x: 0, y: 0, intensity: 0 }
};

// -------------------- util --------------------
function $(id) { return document.getElementById(id); }
function pctToPwm(pct) { return Math.round((pct / 100) * 255); }
function safe(v, d) { return (v === undefined || v === null || Number.isNaN(v)) ? d : v; }
function fmtTemp(v) { return (v === undefined || v === null || Number.isNaN(v)) ? '--' : v.toFixed(1) + ' °C'; }
function fmtHum(v) { return (v === undefined || v === null || Number.isNaN(v)) ? '--' : v.toFixed(0) + ' %'; }
function fmtPres(v) { return (v === undefined || v === null || Number.isNaN(v)) ? '--' : v.toFixed(0) + ' hPa'; }
function fmtDist(mm) { return (mm === undefined || mm === null || !mm) ? '--' : (mm / 10).toFixed(1) + ' cm'; }
function escapeHtml(str) {
  return String(str).replace(/[&<>"']/g, c => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;' }[c]));
}

// -------------------- tabs --------------------
function initTabs() {
  const tabs = Array.from(document.querySelectorAll('.tab'));
  tabs.forEach(tab => {
    tab.addEventListener('click', () => selectTab(tab.getAttribute('data-tab')));
  });
  selectTab('home');
}

function selectTab(name) {
  state.activeTab = name;
  document.querySelectorAll('.tab').forEach(t => {
    const active = t.getAttribute('data-tab') === name;
    t.classList.toggle('active', active);
    t.setAttribute('aria-selected', active ? 'true' : 'false');
  });
  ['home', 'dl', 'config'].forEach(n => {
    const p = $('panel-' + n);
    if (p) {
      p.classList.toggle('active', n === name);
      p.classList.toggle('hidden', n !== name);
    }
  });
  if (name === 'home') requestAnimationFrame(() => { measureJoystick('home'); });
  if (name === 'dl') requestAnimationFrame(() => { measureJoystick('dl'); });
  try { history.replaceState(null, '', '#' + name); } catch (e) {}
  try { localStorage.setItem('irobot-tab', name); } catch (e) {}
}

// -------------------- conectado --------------------
function setOnline(ok) {
  state.online = !!ok;
  const badge = $('badge-conn');
  if (badge) {
    badge.textContent = ok ? 'Conectado' : 'Desconectado';
    badge.className = 'badge ' + (ok ? 'online' : 'offline');
  }
  const homeConn = $('home-conn');
  if (homeConn) {
    homeConn.textContent = ok ? 'Conectado' : 'Desconectado';
    homeConn.className = 'status-v ' + (ok ? 'online' : 'offline');
  }
  const dlConn = $('dl-tel-conn');
  if (dlConn) {
    dlConn.textContent = ok ? 'Online' : 'Offline';
    dlConn.className = 'tel-value ' + (ok ? 'online' : 'offline');
  }
}

// -------------------- joystick --------------------
function measureJoystick(key) {
  const j = joyState[key];
  if (!j.base) return;
  const r = j.base.getBoundingClientRect();
  j.cx = r.left + r.width / 2;
  j.cy = r.top + r.height / 2;
  j.radius = Math.max(10, r.width / 2 - j.knobRadius - 4);
}

function pointerPos(e) {
  if (e.touches && e.touches.length) return { x: e.touches[0].clientX, y: e.touches[0].clientY, id: e.touches[0].identifier };
  return { x: e.clientX, y: e.clientY, id: null };
}

function setKnob(key, x, y) {
  const j = joyState[key];
  if (!j.base || !j.knob) return;
  let dx = x - j.cx, dy = y - j.cy;
  let dist = Math.hypot(dx, dy);
  let cx = x, cy = y;
  if (dist > j.radius && dist > 0) {
    const ratio = j.radius / dist;
    cx = j.cx + dx * ratio;
    cy = j.cy + dy * ratio;
  }
  const ox = cx - j.cx, oy = cy - j.cy;
  j.knob.style.transform = 'translate(calc(-50% + ' + ox + 'px), calc(-50% + ' + oy + 'px))';
  const norm = Math.min(dist / j.radius, 1);
  j.x = ox / j.radius;
  j.y = -oy / j.radius;
  j.intensity = norm < 0.07 ? 0 : norm;
  if (j.intensity === 0) { j.x = 0; j.y = 0; }
  updateJoyReadout(key);
  sendJoystick(key);
}

function joyLabel(x, y, i) {
  if (i < 0.07) return 'Parado';
  if (Math.abs(y) > Math.abs(x)) return y > 0 ? 'Frente' : 'Trás';
  return x > 0 ? 'Direita' : 'Esquerda';
}

function updateJoyReadout(key) {
  const j = joyState[key];
  if (j.dirEl) j.dirEl.textContent = joyLabel(j.x, j.y, j.intensity);
  if (j.intEl) j.intEl.textContent = 'Int: ' + Math.round(j.intensity * 100) + '%';
}

function resetJoystick(key) {
  const j = joyState[key];
  if (j.knob) j.knob.style.transform = 'translate(-50%, -50%)';
  j.x = 0; j.y = 0; j.intensity = 0; j.active = false; j.touchId = null;
  updateJoyReadout(key);
  sendDifferential(0, 0);
}

function sendJoystick(key) {
  if (!state.powerOn) return;
  const j = joyState[key];
  const scale = state.speedPct / 100;
  let left = j.y * j.intensity - j.x * j.intensity;
  let right = j.y * j.intensity + j.x * j.intensity;
  left = Math.max(-1, Math.min(1, left));
  right = Math.max(-1, Math.min(1, right));
  sendDifferential(Math.round(left * 100 * scale), Math.round(right * 100 * scale));
}

function bindJoystick(key, baseId, knobId, dirId, intId) {
  const j = joyState[key];
  j.base = $(baseId);
  j.knob = $(knobId);
  j.dirEl = $(dirId);
  j.intEl = $(intId);
  if (!j.base || !j.knob) return;
  j.base.style.touchAction = 'none';
  j.knob.style.touchAction = 'none';

  const start = e => {
    e.preventDefault(); e.stopPropagation();
    const p = pointerPos(e);
    measureJoystick(key);
    j.active = true; j.touchId = p.id;
    setKnob(key, p.x, p.y);
  };
  const move = e => {
    if (!j.active) return;
    const p = pointerPos(e);
    if (j.touchId !== null && p.id !== j.touchId) return;
    e.preventDefault(); e.stopPropagation();
    setKnob(key, p.x, p.y);
  };
  const end = e => {
    if (!j.active) return;
    if (e.touches && e.touches.length) {
      const t = e.touches[0];
      if (j.touchId !== null && t.identifier !== j.touchId) return;
    }
    e.preventDefault(); e.stopPropagation();
    resetJoystick(key);
  };

  j.base.addEventListener('mousedown', start);
  j.base.addEventListener('touchstart', start, { passive: false });
  window.addEventListener('mousemove', move);
  window.addEventListener('touchmove', move, { passive: false });
  window.addEventListener('mouseup', end);
  window.addEventListener('touchend', end, { passive: false });
  window.addEventListener('touchcancel', end, { passive: false });
  window.addEventListener('resize', () => measureJoystick(key));
}

// -------------------- comandos --------------------
async function sendDifferential(left, right) {
  if (left === state.lastSent.left && right === state.lastSent.right) return;
  try {
    await API.post('/api/motor/move', { leftSpeed: left, rightSpeed: right });
    state.lastSent = { left, right, dir: '', speed: 0 };
    setOnline(true);
  } catch (err) {
    setOnline(false);
  }
}

async function sendDir(dir) {
  const speed = pctToPwm(state.speedPct);
  try {
    await API.post('/api/motor/move', { direction: dir, speed });
    state.lastSent = { left: 0, right: 0, dir, speed };
    setOnline(true);
    updateStatusLocal(dir);
  } catch (err) {
    setOnline(false);
  }
}

async function stopAll() {
  try {
    await API.post('/api/motor/stop', {});
    setOnline(true);
  } catch (err) {
    setOnline(false);
  }
  state.lastSent = { left: 0, right: 0, dir: '', speed: 0 };
  updateStatusLocal('stop');
}

function updateStatusLocal(dir) {
  const label = { frente: 'Andando', tras: 'Andando', esquerda: 'Andando', direita: 'Andando', stop: 'Parado' }[dir] || 'Parado';
  const el = $('home-state');
  if (el) el.textContent = label;
  const mov = $('home-moving');
  if (mov) mov.textContent = label;
  const dlSt = $('dl-tel-status');
  if (dlSt) dlSt.textContent = ({ frente: 'Frente', tras: 'Trás', esquerda: 'Esquerda', direita: 'Direita', stop: 'Parado' }[dir] || 'Parado');
}

async function toggleAuto() {
  const turnOn = !state.autoOn;
  try {
    const res = await API.post('/api/auto', { enabled: turnOn });
    state.autoOn = res.enabled === true;
    setOnline(true);
    syncAutoButtons();
    if (!state.autoOn) { resetJoystick('home'); resetJoystick('dl'); }
    fetchSensors();
  } catch (err) {
    setOnline(false);
  }
}

function syncAutoButtons() {
  const txt = 'Modo Autonomo: ' + (state.autoOn ? 'ON' : 'OFF');
  ['btn-home-auto', 'dl-auto', 'btn-auto'].forEach(id => {
    const el = $(id);
    if (el) {
      el.textContent = txt;
      el.classList.toggle('on', state.autoOn);
    }
  });
}

// -------------------- sensores --------------------
function buildAdviceMessages(s) {
  const a = s.alerts || {};
  const msgs = [];
  if (a.fire) msgs.push('INCENDIO: temperatura subiu rapido — verifique o ambiente!');
  if (a.rainSensor) msgs.push('Chuva agora (sensor molhado)');
  else if (a.rain) msgs.push('Pode chover: pressao caiu ' + Math.abs(a.trend3h || 0).toFixed(1) + ' hPa/3h');
  if (a.hot) msgs.push('Calor: ligue o ar-condicionado');
  if (a.cold) msgs.push('Frio: desligue o ar-condicionado');
  if (a.dry) msgs.push('Ar seco: beba agua');
  return msgs;
}

function renderTicker(s, msgs) {
  const el = $('ticker-text');
  if (!el) return;
  const temp = fmtTemp(s.temperature);
  const hum = fmtHum(s.humidity);
  const dist = fmtDist(s.distanceMm);
  const base = 'Temp ' + temp + ' | Umid ' + hum + ' | Dist ' + dist;
  el.textContent = msgs.length ? (msgs.join('  •  ') + '  |  ' + base) : (base + '  |  Nenhum aviso');
}

function renderHomeSensors(s, msgs) {
  $('home-temp').textContent = fmtTemp(s.temperature);
  $('home-hum').textContent = fmtHum(s.humidity);
  $('home-pres').textContent = fmtPres(s.pressure);
  $('home-dist').textContent = fmtDist(s.distanceMm);
  if (s.alerts && s.alerts.fire && !state.lastFire) notifyFire();
  state.lastFire = !!(s.alerts && s.alerts.fire);
}

function renderDlSensors(s, msgs) {
  $('dl-temp').textContent = fmtTemp(s.temperature);
  $('dl-hum').textContent = fmtHum(s.humidity);
  $('dl-pres').textContent = fmtPres(s.pressure);
  $('dl-dist').textContent = fmtDist(s.distanceMm);
  const present = [];
  if (s.aht) present.push('AHT20');
  if (s.bmp) present.push('BMP280');
  if (s.tof) present.push('VL53L0X');
  const st = $('dl-env-status');
  if (st) st.textContent = (present.length ? 'sensores: ' + present.join(', ') : 'sensores: nenhum') +
    (s.auto ? ' | autonomo: ' + s.autoState + ' (' + safe(s.avoidCount, 0) + '/100)' : '');
  const list = $('dl-advice-list');
  if (list) {
    if (!msgs.length) list.innerHTML = '<li class="advice-none">Nenhum aviso no momento</li>';
    else list.innerHTML = msgs.map(m => '<li>' + escapeHtml(m) + '</li>').join('');
  }
  const tr = $('dl-trend-status');
  if (tr) {
    const a = s.alerts || {};
    if (a.trendReady) tr.textContent = 'tendencia: ' + (a.trend30m || 0).toFixed(1) + ' hPa/30min | ' + (a.trend3h || 0).toFixed(1) + ' hPa/3h';
    else tr.textContent = 'tendencia: coletando dados...';
  }
}

async function fetchSensors() {
  try {
    const s = await API.get('/api/sensors');
    setOnline(true);
    state.autoOn = s.auto === true;
    syncAutoButtons();
    const msgs = buildAdviceMessages(s);
    renderTicker(s, msgs);
    renderHomeSensors(s, msgs);
    renderDlSensors(s, msgs);
    if (s.stuck && s.stuckSeq !== state.lastStuckSeq) {
      state.lastStuckSeq = s.stuckSeq;
      notifyStuck();
    }
  } catch (err) {
    setOnline(false);
    const el = $('ticker-text');
    if (el) el.textContent = 'Falha ao ler sensores: ' + (err.message || 'erro');
  }
}

async function fetchStatus() {
  try {
    const st = await API.get('/api/motor/status');
    setOnline(true);
    const dirMap = { 0: 'Parado', 1: 'Frente', 2: 'Trás', 3: 'Esquerda', 4: 'Direita' };
    const dir = dirMap[st.direction] || 'Parado';
    const homeState = $('home-state');
    if (homeState) homeState.textContent = st.moving ? 'Andando' : 'Parado';
    const homeMoving = $('home-moving');
    if (homeMoving) homeMoving.textContent = st.moving ? 'Andando (' + dir + ')' : 'Parado';
    const dlSt = $('dl-tel-status'); if (dlSt) dlSt.textContent = dir;
    const dlL = $('dl-tel-left'); if (dlL) dlL.textContent = st.leftSpeed;
    const dlR = $('dl-tel-right'); if (dlR) dlR.textContent = st.rightSpeed;
    const dlM = $('dl-tel-moving'); if (dlM) { dlM.textContent = st.moving ? 'Sim' : 'Nao'; dlM.className = 'tel-value ' + (st.moving ? 'online' : 'offline'); }
    const dlE = $('dl-tel-api-error'); if (dlE) { dlE.textContent = 'OK'; dlE.className = 'tel-value online'; }
  } catch (err) {
    setOnline(false);
    const dlE = $('dl-tel-api-error'); if (dlE) { dlE.textContent = err.message || 'erro'; dlE.className = 'tel-value offline'; }
  }
}

// -------------------- alertas --------------------
function notifyFire() {
  const el = $('alert-fire'); if (el) el.hidden = false;
  try { if (navigator.vibrate) navigator.vibrate([900, 200, 900, 200, 1500]); } catch (e) {}
  try {
    const Ctx = window.AudioContext || window.webkitAudioContext;
    if (Ctx) {
      const ctx = new Ctx();
      [0, 0.4, 0.8, 1.2].forEach(t => {
        const o = ctx.createOscillator(), g = ctx.createGain();
        o.type = 'sawtooth'; o.frequency.value = 1200; g.gain.value = 0.09;
        o.connect(g); g.connect(ctx.destination);
        o.start(ctx.currentTime + t); o.stop(ctx.currentTime + t + 0.25);
      });
    }
  } catch (e) {}
}

function notifyStuck() {
  const el = $('alert-stuck'); if (el) el.hidden = false;
  try { if (navigator.vibrate) navigator.vibrate([500, 200, 500, 200, 800]); } catch (e) {}
}

// -------------------- config --------------------
async function loadConfig() {
  try {
    const cfg = await API.get('/api/config');
    state.config = cfg;
    const set = (id, v) => { const el = $(id); if (el) el.value = v; };
    set('cfg-pwm-min', cfg.pwmMin); set('cfg-pwm-max', cfg.pwmMax); set('cfg-pwm-default', cfg.pwmDefault);
    set('dl-pwm-min', cfg.pwmMin); set('dl-pwm-max', cfg.pwmMax); set('dl-pwm-default', cfg.pwmDefault);
    const mot = cfg.motorsEnabled === true || cfg.motorsEnabled === 1 || cfg.motorsEnabled === 'true';
    if ($('cfg-motors-enabled')) $('cfg-motors-enabled').checked = mot;
    if ($('dl-motors-enabled')) $('dl-motors-enabled').checked = mot;
    setOnline(true);
  } catch (err) { setOnline(false); }
}

async function saveConfig(prefix) {
  const get = id => { const el = $(id); return el ? el.value : undefined; };
  const payload = {
    pwmMin: parseInt(get(prefix + '-pwm-min')) || 0,
    pwmMax: parseInt(get(prefix + '-pwm-max')) || 255,
    pwmDefault: parseInt(get(prefix + '-pwm-default')) || 150,
    motorsEnabled: $(prefix + '-motors-enabled') ? $(prefix + '-motors-enabled').checked : true
  };
  try { await API.post('/api/config', payload); setOnline(true); await loadConfig(); }
  catch (err) { setOnline(false); }
}

async function fetchSystemInfo(target) {
  try {
    const info = await API.get('/api/system/info');
    const el = $(target);
    if (el) el.textContent = JSON.stringify(info, null, 2);
    setOnline(true);
  } catch (err) { setOnline(false); }
}

function initWifiMode() {
  const sel = $('cfg-wifi-mode');
  if (!sel) return;
  const apply = () => {
    const isSta = sel.value === 'sta';
    ['cfg-wifi-sta-row', 'cfg-wifi-pass-row'].forEach(id => { const el = $(id); if (el) el.style.display = isSta ? 'flex' : 'none'; });
    ['cfg-wifi-ap-row', 'cfg-wifi-ap-pass-row'].forEach(id => { const el = $(id); if (el) el.style.display = isSta ? 'none' : 'flex'; });
  };
  sel.addEventListener('change', apply);
  apply();
}

// -------------------- tela cheia / PWA --------------------
function initFullscreen() {
  const cb = $('fs-toggle');
  if (!cb) return;
  cb.addEventListener('change', async () => {
    try {
      if (cb.checked) {
        const el = document.documentElement;
        if (el.requestFullscreen) await el.requestFullscreen();
        else if (el.webkitRequestFullscreen) await el.webkitRequestFullscreen();
        try { await screen.orientation.lock('landscape'); } catch (e) {}
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
  window.addEventListener('beforeinstallprompt', e => {
    e.preventDefault();
    window.__deferredPrompt = e;
  });
}

// -------------------- init --------------------
function init() {
  initTabs();
  initWifiMode();
  initFullscreen();
  initPwa();

  bindJoystick('home', 'joy-base', 'joy-knob', 'joy-dir', 'joy-int');
  bindJoystick('dl', 'dl-joy-base', 'dl-joy-knob', 'dl-joy-dir', 'dl-joy-int');
  requestAnimationFrame(() => { measureJoystick('home'); measureJoystick('dl'); });

  document.querySelectorAll('.dpad-btn[data-dir]').forEach(btn => {
    btn.addEventListener('click', e => {
      e.preventDefault();
      const dir = btn.getAttribute('data-dir');
      const key = btn.id.startsWith('dl-') ? 'dl' : 'home';
      resetJoystick(key);
      sendDir(dir);
      document.querySelectorAll('.dpad-btn').forEach(b => b.classList.toggle('active', b === btn));
      setTimeout(() => btn.classList.remove('active'), 200);
    });
  });

  const stopBtns = [$('btn-stop'), $('dl-stop')];
  stopBtns.forEach(b => { if (b) b.addEventListener('click', () => {
    resetJoystick('home'); resetJoystick('dl'); stopAll();
  }); });

  ['btn-home-auto', 'dl-auto', 'btn-auto'].forEach(id => {
    const el = $(id); if (el) el.addEventListener('click', toggleAuto);
  });

  const bindSpeed = (sliderId, valId) => {
    const sl = $(sliderId), vv = $(valId);
    if (!sl) return;
    sl.addEventListener('input', () => {
      state.speedPct = parseInt(sl.value) || 0;
      document.querySelectorAll('#home-speed, #dl-speed').forEach(s => { if (s) s.value = state.speedPct; });
      document.querySelectorAll('#home-speed-val, #dl-speed-val').forEach(s => { if (s) s.textContent = state.speedPct + '%'; });
    });
  };
  bindSpeed('home-speed', 'home-speed-val');
  bindSpeed('dl-speed', 'dl-speed-val');

  const bindDiffSlider = (id, valId, which) => {
    const sl = $(id), vv = $(valId);
    if (!sl) return;
    sl.addEventListener('input', () => {
      const v = parseInt(sl.value) || 0;
      if (vv) vv.textContent = v;
      state[which] = v;
    });
  };
  bindDiffSlider('dl-left', 'dl-left-val', 'leftPct');
  bindDiffSlider('dl-right', 'dl-right-val', 'rightPct');
  const apply = $('dl-apply-diff');
  if (apply) apply.addEventListener('click', () => sendDifferential(state.leftPct, state.rightPct));

  const savePwm = $('cfg-save-pwm'); if (savePwm) savePwm.addEventListener('click', () => saveConfig('cfg'));
  const saveDl = $('dl-save-config'); if (saveDl) saveDl.addEventListener('click', () => saveConfig('dl'));
  const sysA = $('cfg-sys-info'); if (sysA) sysA.addEventListener('click', () => fetchSystemInfo('cfg-system-info'));
  const sysB = $('dl-sys-info'); if (sysB) sysB.addEventListener('click', () => fetchSystemInfo('dl-system-info'));
  const restart = $('cfg-restart'); if (restart) restart.addEventListener('click', async () => {
    try { await fetch('/api/system/restart', { method: 'POST' }); } catch (e) {}
  });
  const saveWifi = $('cfg-save-wifi'); if (saveWifi) saveWifi.addEventListener('click', async () => {
    try {
      const mode = $('cfg-wifi-mode').value;
      const payload = mode === 'sta'
        ? { mode, ssid: $('cfg-wifi-ssid').value, pass: $('cfg-wifi-pass').value }
        : { mode, ssid: $('cfg-ap-ssid').value, pass: $('cfg-ap-pass').value };
      await API.post('/api/wifi', payload);
      setOnline(true);
    } catch (err) { setOnline(false); }
  });

  const fa = $('btn-fire-ok'); if (fa) fa.addEventListener('click', () => { const e = $('alert-fire'); if (e) e.hidden = true; });
  const sa = $('btn-alert-ok'); if (sa) sa.addEventListener('click', () => { const e = $('alert-stuck'); if (e) e.hidden = true; });
  const fb = $('dl-btn-fire-ok'); if (fb) fb.addEventListener('click', () => { const e = $('dl-alert-fire'); if (e) e.hidden = true; });
  const sb = $('dl-btn-alert-ok'); if (sb) sb.addEventListener('click', () => { const e = $('dl-alert-stuck'); if (e) e.hidden = true; });

  loadConfig();
  fetchStatus();
  fetchSensors();
  setInterval(fetchStatus, 1000);
  setInterval(fetchSensors, 1500);

  try {
    const saved = localStorage.getItem('irobot-tab');
    if (saved) selectTab(saved);
  } catch (e) {}
  if (location.hash) {
    const h = location.hash.replace('#', '');
    if (['home', 'dl', 'config'].includes(h)) selectTab(h);
  }
}

document.addEventListener('DOMContentLoaded', init);

)rawliteral";
static const char* teste_html = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-BR">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no">
<title>iRobot - Teste</title>
<style>
  * { box-sizing: border-box; }
  html, body {
    margin: 0; height: 100%; background: #101418; color: #e8eef5;
    font-family: system-ui, -apple-system, "Segoe UI", Roboto, sans-serif;
    -webkit-user-select: none; user-select: none; touch-action: manipulation;
  }
  body {
    display: flex; flex-direction: column; align-items: center;
    justify-content: center; gap: 16px; padding: 16px;
  }
  h1 { margin: 0; font-size: 1.4rem; font-weight: 600; letter-spacing: 0.04em; color: #8fd7ff; }
  .pad {
    display: grid;
    grid-template-columns: repeat(3, minmax(88px, 120px));
    grid-template-rows: repeat(3, minmax(88px, 120px));
    gap: 10px;
  }
  .btn {
    display: flex; flex-direction: column; align-items: center; justify-content: center;
    gap: 6px; border: 0; border-radius: 16px; background: #1d2733; color: #e8eef5;
    font-size: 1.6rem; font-family: inherit; cursor: pointer;
    -webkit-tap-highlight-color: transparent; transition: background 0.1s, transform 0.05s;
  }
  .btn span { font-size: 0.85rem; color: #9fb2c6; }
  .btn:active, .btn.active { background: #2f9fd8; transform: scale(0.97); }
  .btn:active span, .btn.active span { color: #eaf7ff; }
  .stop { background: #3a1f24; }
  .stop span { color: #ffb3b3; }
  .stop:active, .stop.active { background: #d64553; }
  .btn[data-dir="frente"] { grid-area: 1 / 2; }
  .btn[data-dir="esquerda"] { grid-area: 2 / 1; }
  .btn[data-dir="stop"] { grid-area: 2 / 2; }
  .btn[data-dir="direita"] { grid-area: 2 / 3; }
  .btn[data-dir="tras"] { grid-area: 3 / 2; }
  #status { margin: 0; min-height: 1.4em; font-size: 0.95rem; color: #9fb2c6; text-align: center; }
  #status.error { color: #ff8a8a; }
  .back { color: #9fb2c6; font-size: 0.9rem; text-decoration: none; }
  #amb { margin: 0; font-size: 0.9rem; color: #9fb2c6; text-align: center; font-variant-numeric: tabular-nums; }
  #adv { margin: 0; font-size: 0.9rem; color: #ffd479; text-align: center; max-width: 340px; line-height: 1.3; }
  #adv.crit { color: #ff8a8a; font-weight: 600; }
  #auto {
    border: 1px solid #2f4257; background: #16202b; color: #cfe3f5;
    font-family: inherit; font-size: 0.95rem; padding: 10px 18px; border-radius: 12px;
    cursor: pointer; -webkit-tap-highlight-color: transparent;
  }
  #auto.on { background: #1f7a4d; border-color: #2fa76a; color: #eafff3; }
</style>
</head>
<body>
<h1>iRobot - Teste</h1>

<div class="pad">
  <button class="btn" data-dir="frente">&#9650;<span>Frente</span></button>
  <button class="btn" data-dir="esquerda">&#9664;<span>Esquerda</span></button>
  <button class="btn stop" data-dir="stop">&#9632;<span>Parar</span></button>
  <button class="btn" data-dir="direita">&#9654;<span>Direita</span></button>
  <button class="btn" data-dir="tras">&#9660;<span>Tr&#225;s</span></button>
</div>

<p id="status">toque em um bot&#227;o</p>
<p id="amb">ambiente: lendo...</p>
<p id="adv"></p>
<button id="auto" type="button">Aut&#244;nomo: OFF</button>
<a class="back" href="/">voltar ao painel completo</a>

<script>
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
    btn.addEventListener('click', function (event) {
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
    });
  });

  var ambEl = document.getElementById('amb');
  var advEl = document.getElementById('adv');
  var autoBtn = document.getElementById('auto');
  var autoOn = false;
  var lastStuckSeq = 0;

  function refreshSensors() {
    fetch('/api/sensors', { cache: 'no-store' }).then(function (res) {
      if (!res.ok) throw new Error('HTTP ' + res.status);
      return res.json();
    }).then(function (s) {
      var temp = (s.temperature === undefined) ? '--' : s.temperature.toFixed(1) + ' C';
      var hum = (s.humidity === undefined) ? '--' : s.humidity.toFixed(0) + ' %';
      var pres = (s.pressure === undefined) ? '--' : s.pressure.toFixed(0) + ' hPa';
      var dist = s.distanceMm ? (s.distanceMm / 10).toFixed(1) + ' cm' : '--';
      ambEl.textContent = temp + ' | ' + hum + ' | ' + pres + ' | dist ' + dist;
      showAdvice(s, hum);
      autoOn = s.auto === true;
      if (s.stuck && s.stuckSeq !== lastStuckSeq) {
        lastStuckSeq = s.stuckSeq;
        alertStuck();
      }
      autoBtn.textContent = 'Aut\u00f4nomo: ' + (autoOn ? 'ON' : 'OFF');
      autoBtn.className = autoOn ? 'on' : '';
    }).catch(function (err) {
      ambEl.textContent = 'ambiente: falha (' + err.message + ')';
    });
  }

  var lastFire = false;

  function showAdvice(s, hum) {
    var a = s.alerts || {};
    var msgs = [];
    if (a.fire) msgs.push('INCENDIO! temperatura subiu rapido - verifique o ambiente');
    if (a.rainSensor) msgs.push('chuva agora (sensor molhado)');
    else if (a.rain) msgs.push('pode chover - pressao caiu ' + Math.abs(a.trend3h || 0).toFixed(1) + ' hPa/3h');
    if (a.hot) msgs.push('calor - ligar o ar-condicionado');
    if (a.cold) msgs.push('frio - desligar o ar-condicionado');
    if (a.dry) msgs.push('ar seco - beba agua');
    advEl.textContent = msgs.length ? msgs.join(' | ') : '';
    advEl.className = a.fire ? 'crit' : '';
    if (a.fire && !lastFire) alertFire();
    lastFire = !!a.fire;
  }

  function alertFire() {
    setStatus('ALERTA: possivel incendio', true);
    try { if (navigator.vibrate) navigator.vibrate([900, 200, 900, 200, 1500]); } catch (e) {}
    try {
      var Ctx = window.AudioContext || window.webkitAudioContext;
      if (Ctx) {
        var ctx = new Ctx();
        [0, 0.4, 0.8, 1.2].forEach(function (t) {
          var osc = ctx.createOscillator();
          var gain = ctx.createGain();
          osc.type = 'sawtooth';
          osc.frequency.value = 1200;
          gain.gain.value = 0.09;
          osc.connect(gain);
          gain.connect(ctx.destination);
          osc.start(ctx.currentTime + t);
          osc.stop(ctx.currentTime + t + 0.25);
        });
      }
    } catch (e) {}
  }

  function alertStuck() {
    setStatus('ALERTA: robo enroscado - autonomo desligado', true);
    try { if (navigator.vibrate) navigator.vibrate([500, 200, 500, 200, 800]); } catch (e) {}
    try {
      var Ctx = window.AudioContext || window.webkitAudioContext;
      if (Ctx) {
        var ctx = new Ctx();
        [0, 0.35, 0.7].forEach(function (t) {
          var osc = ctx.createOscillator();
          var gain = ctx.createGain();
          osc.type = 'square';
          osc.frequency.value = 880;
          gain.gain.value = 0.08;
          osc.connect(gain);
          gain.connect(ctx.destination);
          osc.start(ctx.currentTime + t);
          osc.stop(ctx.currentTime + t + 0.18);
        });
      }
    } catch (e) {}
  }

  autoBtn.addEventListener('click', function () {
    autoBtn.disabled = true;
    fetch('/api/auto', {
      method: 'POST',
      cache: 'no-store',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ enabled: !autoOn })
    }).then(function (res) {
      return res.text().then(function (text) {
        if (!res.ok) throw new Error('HTTP ' + res.status + ' ' + text);
      });
    }).then(function () {
      setStatus(autoOn ? 'autonomo desligado' : 'autonomo ligado');
    }).catch(function (err) {
      setStatus('falha: ' + err.message, true);
    }).then(function () {
      autoBtn.disabled = false;
      refreshSensors();
    });
  });

  setStatus('toque em um bot\u00e3o');
  refreshSensors();
  setInterval(refreshSensors, 1000);
})();
</script>
</body>
</html>

)rawliteral";
static const char* manifest_json = R"rawliteral(
{
  "name": "iRobot - Controle",
  "short_name": "iRobot",
  "description": "Controle e monitoramento do robo iRobot",
  "start_url": "/",
  "scope": "/",
  "display": "fullscreen",
  "orientation": "any",
  "background_color": "#0b1120",
  "theme_color": "#0b1120",
  "icons": [
    {
      "src": "/favicon.svg",
      "sizes": "any",
      "type": "image/svg+xml",
      "purpose": "any maskable"
    }
  ]
}

)rawliteral";
static const char* favicon_svg = R"rawliteral(
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 100 100" width="100" height="100">
  <rect x="10" y="18" width="80" height="66" rx="16" fill="#1d2733" stroke="#2f9fd8" stroke-width="4"/>
  <rect x="22" y="8" width="6" height="14" rx="3" fill="#2f9fd8"/>
  <rect x="72" y="8" width="6" height="14" rx="3" fill="#2f9fd8"/>
  <circle cx="35" cy="46" r="9" fill="#8fd7ff"/>
  <circle cx="65" cy="46" r="9" fill="#8fd7ff"/>
  <path d="M30 66 Q50 80 70 66" stroke="#8fd7ff" stroke-width="4" fill="none" stroke-linecap="round"/>
  <text x="50" y="100" font-size="1">&#129302;</text>
</svg>

)rawliteral";
static const char* sw_js = R"rawliteral(
const CACHE = 'irobot-v6';
const ASSETS = [
  '/', '/style.css', '/app.js', '/manifest.json', '/favicon.svg', '/teste'
];
self.addEventListener('install', e => {
  e.waitUntil(caches.open(CACHE).then(c => c.addAll(ASSETS)).then(() => self.skipWaiting()));
});
self.addEventListener('activate', e => {
  e.waitUntil(caches.keys().then(keys => Promise.all(keys.filter(k => k !== CACHE).map(k => caches.delete(k)))).then(() => self.clients.claim()));
});
self.addEventListener('fetch', e => {
  if (e.request.method !== 'GET') return;
  if (e.request.url.indexOf('/api/') !== -1) return;
  e.respondWith(
    caches.match(e.request).then(cached => {
      if (cached) return cached;
      return fetch(e.request).then(res => {
        if (res.ok) {
          const copy = res.clone();
          caches.open(CACHE).then(c => c.put(e.request, copy));
        }
        return res;
      }).catch(() => new Response('', { status: 503 }));
    })
  );
});

)rawliteral";

#endif // WEB_ASSETS_H