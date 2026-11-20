# 🤖 WindFollow — Suíte de Robótica ESP32

[![GitHub](https://img.shields.io/badge/GitHub-devtiagoabreu%2Fwindfollow-blue?logo=github)](https://github.com/devtiagoabreu/windfollow)
[![Platform](https://img.shields.io/badge/Platform-ESP32%20%7C%20ESP32--S3-orange?logo=espressif)](https://www.espressif.com/)
[![Framework](https://img.shields.io/badge/Framework-Arduino-green?logo=arduino)](https://www.arduino.cc/)
[![Language](https://img.shields.io/badge/Language-C%2B%2B%20%7C%20HTML%20%7C%20CSS%20%7C%20JS-blue)](https://github.com/devtiagoabreu/windfollow)

> **Repositório de firmware e interfaces web para cinco projetos com ESP32:**
> uma câmera seguidora de movimento, um controlador de cortinas, um robô móvel autônomo,
> um módulo de sensores ambientais e um controlador de pan de fluxo de ar.

---

## 👤 Autor

**Tiago de Abreu** — [@devtiagoabreu](https://github.com/devtiagoabreu)

---

## 📋 Visão geral

| Projeto | Placa | Finalidade | Pasta |
|---|---|---|---|
| 🎥 **WindFollow** | LILYGO T-SIMCAM ESP32-S3 | Câmera seguidora por movimento (PAN com motorredutor) | `firmware/windfollow/` |
| 🪟 **WindFollowBlind** | ESP32 DevKit (clássico) | Controle de 2 motores de cortina com 2 botões | `firmware/windfollowblind/` |
| 🤖 **iRobot** | ESP32 DevKit (clássico) | Robô diferencial 2 rodas + caster, sensores e navegação autônoma | `firmware/irobot/` |
| 🌡️ **MSA** | ESP32 DevKit (clássico) | Módulo de sensores ambientais AHT20 + BMP280 com OLED, dashboard responsivo e alertas | `firmware/msa/` |
| 🌬️ **ventiseg** | LILYGO T-SIMCAM ESP32-S3 | Pan do fluxo de ar: 1 motor de passo via driver DM556D 24 VDC, sem laser/relé/fim de curso | `firmware/ventiseg/` |

> O `ventiseg` é a entrega enxuta: uma placa, um motor, um driver. Nesta fase o
> firmware é serial-only (`VENTISEG_ENABLE_GPIO=0`), sem Wi-Fi nem interface web.

Cada firmware inclui:

- 🌐 Interface web embarcada (HTML/CSS/JS embutidos no firmware via `WebAssets.h`)
- 📡 Access Point + mDNS para configuração sem app externo
- ⚙️ Configuração persistida em NVS (Preferences)
- 🔌 API REST para controle e telemetria
- 🧪 Simulador web para testar a interface sem hardware

---

## 📁 Estrutura do repositório

```text
windfollow/
├── 📄 README.md                     # este arquivo
├── 📁 docs/                         # documentação técnica e devlog
│   ├── hardware.md
│   ├── ideias_projeto_01.md
│   ├── ideias_projeto_02.md
│   ├── IROBOT_NAVEGACAO_E_SENSORES.md
│   └── DEVLOG.md                    # histórico de desenvolvimento
├── 📁 firmware/
│   ├── 📁 windfollow/               # 🎥 câmera seguidora (ESP32-S3)
│   ├── 📁 windfollowblind/          # 🪟 cortinas (ESP32 clássico)
│   ├── 📁 irobot/                   # 🤖 robô autônomo (ESP32 clássico)
│   ├── 📁 msa/                      # 🌡️ sensores ambientais (ESP32 clássico)
│   └── 📁 ventiseg/                 # 🌬️ pan do fluxo de ar (ESP32-S3 + DM556D)
├── 📁 simulator/                    # 🧪 interfaces web para teste no PC
├── 📁 tools/                        # 🛠️ scripts de flash e commit
└── 📁 graphify-out/                 # 📊 grafo de conhecimento do código
```

---

# 🎥 1. WindFollow — Câmera seguidora

Câmera que detecta movimento e gira o suporte (PAN) para acompanhar o alvo, com zona morta e histórico de centroid.

## 🧰 Hardware

| Componente | Especificação |
|---|---|
| Placa | LILYGO T-SIMCAM IR-CUT, PCB `S3_CAM_SIM V1.3` (23/06/2022) |
| MCU | ESP32-S3 dual-core LX7, 240 MHz, **16 MB Flash + 8 MB PSRAM** |
| Câmera | OV5640 (320×240 no modo de rastreamento) |
| Driver | L298N (ponte H dupla, 2 A/canal) |
| Motor | Motorredutor CC de veículo elétrico infantil |

## 🔌 GPIOs (T-SIMCAM V1.3)

| Função | GPIO |
|---|---|
| Câmera DVP | XCLK 14, SIOD 4, SIOC 5, D0 11, D1 9, D2 8, D3 10, D4 12, D5 17, D6 16, D7 15, VSYNC 6, HREF 7, PCLK 13 |
| IR-CUT | 18 |
| Microfone | 42, 41, 2 |
| Cartão SD | 40, 38, 39, 47 |
| UART do modem | 43, 44 |
| PCIE/modem | 48 (PCIE_PWR), 45 (TX), 46 (RX) |
| LED | 21 |
| Power latch | 1 |
| Botão | 0 |

**Ligação padrão do L298N (modem 4G desligado):**

| Sinal | GPIO |
|---|---|
| IN1 | 45 |
| IN2 | 46 |
| ENA (PWM) | 21 |

> ⚠️ O firmware mantém GPIO48 em LOW no boot para desligar o modem e liberar 45/46.
> GPIO45/46 são *strapping pins*: o L298N não pode puxá-los para HIGH no boot.

## 🌐 Rede

- 📶 AP: `WindFollow-Setup` / senha `windfollow123` — IP `192.168.4.1`
- 🔖 mDNS: `http://windfollow.local`
- 💾 NVS: namespace `windfollow`

## 🧩 Módulos do firmware

| Arquivo | Função |
|---|---|
| `HardwareConfig.h` | Struct de configuração (GPIOs, tensão, zona morta) |
| `ConfigManager.h/.cpp` | Leitura/gravação em Preferences, migração e validação |
| `L298NDriver.h/.cpp` | Controle do L298N |
| `MotionEngine.h/.cpp` | Movimento com duração de pulso |
| `MotionDetector.h/.cpp` | Detecção por diferença de quadros + centroide |
| `CameraService.h/.cpp` | OV5640, captura JPEG, inicialização sob demanda |
| `DashboardService.h/.cpp` | Servidor HTTP, API REST e assets embarcados |
| `WebAssets.h` | HTML/CSS/JS gerados |
| `windfollow.ino` | Setup/loop, AP + mDNS, câmera, heartbeat |

## ⌨️ Comandos seriais (115200)

`c` inicia a câmera · `i` status · `p` captura · `r` reinicia

---

# 🪟 2. WindFollowBlind — Cortinas

Controle de **2 motores DC via L298N** + **2 botões NA** em ESP32 DevKit, sem câmera. Cada botão alterna seu motor: parado → esquerda → parado → direita → parado → repete.

## 🔌 Hardware

| Componente | GPIO | Função |
|---|---|---|
| Motor 1 IN1 / IN2 | 26 / 25 | Sentido do motor 1 |
| Motor 1 ENA | jumper | Habilitação (sem PWM) |
| Motor 2 IN3 / IN4 | 32 / 33 | Sentido do motor 2 |
| Motor 2 ENB | jumper | Habilitação (sem PWM) |
| Botão 1 | 27 | Alterna sequência do motor 1 |
| Botão 2 | 14 | Alterna sequência do motor 2 |

## 🔁 Comportamento por botão

| Toque | Motor 1 | Motor 2 |
|---|---|---|
| 1 | Esquerda (contínuo) | Esquerda (contínuo) |
| 2 | Parado | Parado |
| 3 | Direita (contínuo) | Direita (contínuo) |
| 4 | Parado | Parado |
| 5+ | Reinicia sequência | Reinicia sequência |

## 🌐 Rede

- 📶 AP: `WindFollowBlind-Setup` / senha `windfollow123` — IP `192.168.4.1`
- 🔖 mDNS: `http://windfollowblind.local` (captive portal)
- 🌐 Dashboard web sem câmera, com setup Wi-Fi

## ⌨️ Comandos seriais (115200)

`i` status · `r` reinicia · `1` motor1 esquerda · `2` motor1 direita · `3` motor2 esquerda · `4` motor2 direita · `0` parar tudo

---

# 🤖 3. iRobot — Robô autônomo

Robô diferencial com **2 rodas motrizes + 1 caster**, sensores ambientais e **navegação autônoma com reconhecimento de terreno**.

## 🧰 Hardware

| Componente | GPIO / endereço |
|---|---|
| Motor esquerdo IN1 / IN2 / ENA (PWM) | 26 / 25 / 27 |
| Motor direito IN3 / IN4 / ENB (PWM) | 32 / 33 / 14 |
| I2C SDA / SCL | 21 / 22 (100 kHz) |
| AHT20 (temp/umidade) | `0x38` |
| BMP280 (pressão) | `0x76` ou `0x77` |
| OLED SSD1306 128×64 (MSA / LILYGO G511) | `0x3C` ou `0x3D` |
| VL53L0X (distância ToF) | `0x29` |
| Placa | ESP32 DevKit clássico, 4 MB Flash |
| FQBN | `esp32:esp32:esp32` |

## 🌐 Rede

- 📶 AP: `Irobot-Setup` / senha `irobot1234`
- 🔖 mDNS: `http://irobot.local`
- 🔄 Modo Wi-Fi selecionável na aba Config: **AP (server)** ou **Cliente (client)** com SSID/senha

## 🧭 Navegação autônoma

O `AutoNavigator` é uma máquina de estados reativa baseada apenas no sensor ToF (sem encoder — heading estimado por tempo de giro):

| Estado | Comportamento |
|---|---|
| 🔄 **Reconhecimento (SCAN)** | Varredura de **360°**: 6 passos de 60°, parando e medindo a cada passo. Leitura sem alvo registra `SCAN_MAX_MM` (8000 mm). |
| 🎯 **Alinhamento (ALIGN_BEST)** | Gira até a direção com a maior distância medida. |
| ⬆️ **Frente (FORWARD)** | Anda reto (PWM 140), para a 50 cm de obstáculo. |
| ⬇️ **Ré (BACKUP)** | Recua 500 ms (800 ms a cada 3 obstáculos). |
| 🔁 **Giro (TURN)** | Pivô no eixo até 1 m livre por 2 leituras. Troca de lado se não houver saída. |
| ⏱️ **Medindo (SETTLE)** | Assenta 250 ms e confirma a clareza. |
| 🛑 **Enroscado** | Após 100 tentativas sem avanço livre, desliga o autônomo e alerta. |

## 🌡️ Avisos ambientais

- 🌧️ Chuva provável (queda de pressão em 3 h)
- 🔥 Possível incêndio (temperatura absoluta ou subida rápida)
- 🔥 Calor > 26 °C / ❄️ Frio < 17 °C
- 💧 Ar seco < 30 %

## 🖥️ Interface web (3 abas)

| Aba | Conteúdo |
|---|---|
| 🏠 **Home** | Tela inicial em paisagem que **preenche um retângulo exato**: joystick à esquerda e D-pad no centro superior; **comandos restantes (Autônomo + Velocidade), sensores e status ocupam a faixa inferior**. Avisos ficam no centro do cabeçalho, badge **Conectado** à direita. Fontes, botões, cards e joystick escalam automaticamente com a resolução (ex.: 667×375, 844×390, 960×540). |
| 🎮 **DL** | Detalhe de locomoção: tela antiga mantida com joystick diferencial, comandos rápidos, sliders de velocidade e motores independentes, PWM, ambiente, avisos e telemetria. |
| ⚙️ **Config** | Wi-Fi (AP/cliente), PWM e status do sistema. |

- 📲 **PWA**: manifest + service worker + ícones PNG 🤖 (32/192/512) para instalar como app em tela cheia
- 📐 Layout responsivo que se molda ao tamanho da tela

### 🧪 Preview local (sem hardware)

Abra `firmware/irobot/preview/index.html` no navegador do computador. A UI usa dados simulados (`preview/mock.js`) e não precisa entrar no AP do robô.

```text
preview/index.html          → cenário padrão
preview/index.html?alerts=1 → avisos de chuva/calor/ar seco no header
preview/index.html?fire=1   → alerta de incêndio
preview/index.html?stuck=1  → alerta de robô enroscado
```

> ⚠️ **Nota PWA**: navegadores só oferecem "Instalar app" em **HTTPS ou localhost**.
> No AP do ESP32 (`http://192.168.4.1`) o botão instalar pode não aparecer.

## ⌨️ Comandos seriais (115200)

`f` frente · `t` trás · `e` esquerda · `d` direita · `s` stop · `1`–`9` velocidade · `i` status · `r` reinicia · `a` autônomo ON/OFF · `m` ambiente + avisos

## 📚 Documentação específica

- [`firmware/irobot/README.md`](firmware/irobot/README.md)
- [`docs/IROBOT_NAVEGACAO_E_SENSORES.md`](docs/IROBOT_NAVEGACAO_E_SENSORES.md)
- [`docs/LICOES_APRENDIDAS.md`](docs/LICOES_APRENDIDAS.md) — acertos, erros e soluções consolidados

---

# 🛠️ Ferramentas e fluxo de trabalho

## 🔍 Detecção de placa (antes de qualquer gravação)

A porta COM **muda a cada replug do cabo**. O script detecta a porta, o chip e a MAC, e bloqueia gravações incompatíveis:

```powershell
powershell -ExecutionPolicy Bypass -File tools/flash.ps1 -Sketch irobot -DetectOnly
powershell -ExecutionPolicy Bypass -File tools/flash.ps1 -Sketch windfollowblind -DetectOnly
powershell -ExecutionPolicy Bypass -File tools/flash.ps1 -Sketch windfollow -DetectOnly
powershell -ExecutionPolicy Bypass -File tools/flash.ps1 -Sketch ventiseg -DetectOnly
```

## ⬆️ Compilar e gravar

```powershell
powershell -ExecutionPolicy Bypass -File tools/flash.ps1 -Sketch irobot
powershell -ExecutionPolicy Bypass -File tools/flash.ps1 -Sketch windfollowblind
powershell -ExecutionPolicy Bypass -File tools/flash.ps1 -Sketch windfollow
powershell -ExecutionPolicy Bypass -File tools/flash.ps1 -Sketch ventiseg
```

## 🔄 Regenerar assets web

```powershell
# WindFollow (raiz)
powershell -ExecutionPolicy Bypass -File tools/generate_web_assets.ps1

# iRobot (dentro de firmware/irobot)
powershell -ExecutionPolicy Bypass -File firmware/irobot/tools/generate_web_assets.ps1

# WindFollowBlind
powershell -ExecutionPolicy Bypass -File tools/generate_web_assets.ps1 -WebDir "firmware/windfollowblind/web" -OutFile "firmware/windfollowblind/WebAssets.h"
```

## 💾 Commit + push + devlog

O script `tools/save.ps1` bloqueia segredos, insere uma entrada no `docs/DEVLOG.md`, commita e faz push:

```powershell
powershell -ExecutionPolicy Bypass -File tools/save.ps1 `
  -Message "feat(projeto): resumo" `
  -Summary "mudanca 1; mudanca 2" `
  -Difficulty "dificuldade -> solucao"
```

---

## 🧪 Simuladores

Abra no navegador para testar as interfaces sem hardware:

- `simulator/index.html` — 🎥 WindFollow
- `simulator/windflollowblind/index.html` — 🪟 WindFollowBlind

---

## 📌 Dados pendentes

- ⚡ Tensão nominal, corrente e corrente de partida do motorredutor do WindFollow
- 🔋 Fonte de alimentação separada para o motor (GND comum)
- 🛑 Limites mecânicos / fins de curso

---

## 📄 Licença

Projeto pessoal de estudo e experimentação — [@devtiagoabreu](https://github.com/devtiagoabreu).

---

<div align="center">

Feito com ❤️ e ☕ por **Tiago de Abreu** — [@devtiagoabreu](https://github.com/devtiagoabreu)

</div>
