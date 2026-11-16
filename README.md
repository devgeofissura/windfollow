# WindFollow

Câmera seguidora por movimento para LILYGO T-SIMCAM ESP32-S3 + L298N + motorredutor CC.

Baseado na documentação em `docs/`:
- `hardware.md` — L298N, T-SIMCAM V1.3, motorredutor
- `ideias_projeto_01.md` — estudo de viabilidade (detecção de movimento, PAN, zona morta)
- `ideias_projeto_02.md` — especificação consolidada (SmartCam OS como base, 7 fases)

## Hardware (do docs/hardware.md)

- Placa: LILYGO T-SIMCAM IR-CUT, PCB `S3_CAM_SIM V1.3`, 23/06/2022
- MCU: ESP32-S3 dual-core LX7, 240 MHz, 16 MB Flash + 8 MB PSRAM
- Câmera: OV5640 (confirmado no hardware, 320x240 no modo de rastreamento)
- Driver: L298N (ponte H dupla, 2 A/canal)
- Motor: motorredutor CC de veículo elétrico infantil (tensão/corrente a confirmar)

## GPIOs da T-SIMCAM V1.3

Câmera DVP: XCLK 14, SIOD 4, SIOC 5, D0 11, D1 9, D2 8, D3 10, D4 12, D5 17, D6 16,
D7 15, VSYNC 6, HREF 7, PCLK 13, IR-CUT 18.  
Microfone: 42, 41, 2. Cartão SD: 40, 38, 39, 47. UART do modem: 43, 44.  
PCIE/modem: 48 (PCIE_PWR), 45 (TX), 46 (RX), LED 21. Power latch: 1. Botão: 0.

**Ligação padrão do L298N neste projeto (modem 4G desligado):**

| Função | GPIO |
|--------|------|
| IN1 | 45 |
| IN2 | 46 |
| ENA (PWM) | 21 |

O firmware mantém GPIO48 em LOW no boot para desligar o modem e liberar 45/46.
GPIO45/46 são strapping pins: o L298N não pode puxá-los para HIGH no boot.
Pinos utilizáveis: 21, 45 e 46. O restante pertence à câmera, flash/PSRAM, SD,
microfone, modem ou botão.

## Rede e interface

- AP: `WindFollow-Setup` / senha `windfollow123`, IP `192.168.4.1`
- mDNS: `http://windfollow.local`
- Configuração salva em NVS (namespace `windfollow`)
- Interface web embarcada com dashboard, câmera e rastreamento por movimento

## Simulador (visualização no computador)

Abra `simulator/index.html` no navegador para ver a interface web sem ESP32.
Inclui fluxo de setup Wi-Fi, acesso via mDNS `windfollow.local` e detecção de movimento pela webcam.

## Firmware (para ESP32 via Arduino IDE)

Pasta `firmware/windfollow/`. Módulos:

| Arquivo | Função |
|---------|--------|
| `HardwareConfig.h` | Struct de configuração (GPIOs, tensão, zona morta, etc.) |
| `ConfigManager.h/.cpp` | Leitura/gravação em Preferences (NVS), migração e validação de GPIO |
| `L298NDriver.h/.cpp` | Controle do L298N (esquerda/direita/parar/PWM) |
| `MotionEngine.h/.cpp` | Gerenciamento de movimento com duração de pulso |
| `MotionDetector.h/.cpp` | Detecção de diferença de quadros + centroide |
| `CameraService.h/.cpp` | Câmera OV5640, captura JPEG, inicialização sob demanda |
| `DashboardService.h/.cpp` | Servidor HTTP, API REST e assets embarcados |
| `WebAssets.h` | HTML/CSS/JS gerados por `tools/generate_web_assets.ps1` |
| `windfollow.ino` | Setup/loop, AP + mDNS, câmera, heartbeat e comandos seriais |

Compilação/gravação (ESP32-S3, 16 MB, PSRAM OPI, USB CDC). **A porta COM muda a
cada replug do cabo**, então o script detecta a porta na hora e confere o chip:

```text
powershell -ExecutionPolicy Bypass -File tools/flash.ps1 -Sketch windfollow
```

Para só listar as portas e o chip de cada uma (sem compilar/gravar):

```text
powershell -ExecutionPolicy Bypass -File tools/flash.ps1 -Sketch windfollow -DetectOnly
```

Comandos pela serial (115200): `c` inicia a câmera, `i` mostra status, `p` captura, `r` reinicia.

Antes de regenerar o firmware após editar `web/`, rode:

```text
powershell -ExecutionPolicy Bypass -File tools/generate_web_assets.ps1
```

## Dados pendentes antes de energizar

- Tensão nominal, corrente e corrente de partida do motorredutor
- Fonte de alimentação separada para o motor (GND comum)
- Limites mecânicos / fins de curso

---

# WindFollowBlind (subprojeto — sem câmera)

Pasta `firmware/windfollowblind/` e `simulator/windflollowblind/`.

Controle de **2 motores DC via L298N** + **2 botões NA** em um ESP32 DevKit (não usa a T-SIMCAM nem câmera). Cada botão alterna seu próprio motor entre: parado → esquerda (contínuo) → parado → direita (contínuo) → parado → repete. Mantém a mesma interface web (dashboard, configuração de GPIOs, Wi-Fi AP/mDNS) do WindFollow, removendo apenas a parte de câmera e rastreamento por movimento.

## Hardware (do docs/documentacao_hardware_esp32_l298n.md)

| Componente | GPIO | Função |
|------------|------|--------|
| Motor 1 IN1 | 26 | Sentido 1 do motor 1 |
| Motor 1 IN2 | 25 | Sentido 2 do motor 1 |
| Motor 1 ENA | jumper | Habilitação (sem PWM) |
| Motor 2 IN3 | 32 | Sentido 1 do motor 2 |
| Motor 2 IN4 | 33 | Sentido 2 do motor 2 |
| Motor 2 ENB | jumper | Habilitação (sem PWM) |
| Botão 1 | 27 | Alterna sequencia do motor 1 |
| Botão 2 | 14 | Alterna sequencia do motor 2 |

## Comportamento por botão

Os motores giram **continuamente** até receber um comando de parada. Cada toque no botão alterna o estado do motor correspondente.

| Toque | Motor 1 | Motor 2 |
|-------|---------|---------|
| 1 | Gira para a esquerda (contínuo) | Gira para a esquerda (contínuo) |
| 2 | Para | Para |
| 3 | Gira para a direita (contínuo) | Gira para a direita (contínuo) |
| 4 | Para | Para |
| 5+ | Reinicia a sequência | Reinicia a sequência |

## Rede e interface

- AP: `WindFollowBlind-Setup` / senha `windfollow123`, IP `192.168.4.1`
- mDNS: `http://windfollowblind.local` (captive portal: qualquer navegador conectado ao AP é redirecionado automaticamente para esta página)
- Interface web embarcada (sem câmera): dashboard de motores, configuração de GPIOs, setup Wi-Fi

## Simulador

Abra `simulator/windflollowblind/index.html` no navegador para simular os 2 motores e a interface sem ESP32.

## Firmware (ESP32 DevKit via Arduino IDE)

FQBN: `esp32:esp32:esp32`. A porta COM é detectada na hora (nunca fixa):

```text
powershell -ExecutionPolicy Bypass -File tools/flash.ps1 -Sketch windfollowblind
```

Antes de regenerar o firmware após editar `firmware/windfollowblind/web/`, rode:

```text
powershell -ExecutionPolicy Bypass -File tools/generate_web_assets.ps1 -WebDir "firmware/windfollowblind/web" -OutFile "firmware/windfollowblind/WebAssets.h"
```

Comandos pela serial (115200): `i` status, `r` reinicia, `1` motor1 esquerda, `2` motor1 direita, `3` motor2 esquerda, `4` motor2 direita, `0` parar tudo.

---

# iRobot (subprojeto — robô diferencial de 2 rodas + caster)

Pasta `firmware/irobot/`. Mesmo ESP32 DevKit clássico do WindFollowBlind, sem botões
e sem câmera: 2 motores DC (uma roda por motor) + roda caster livre na outra ponta.

| Sinal | GPIO |
|---|---|
| Motor esquerdo IN1 / IN2 / ENA (PWM) | 26 / 25 / 27 |
| Motor direito IN3 / IN4 / ENB (PWM) | 32 / 33 / 14 |

- AP: `Irobot-Setup` / senha `irobot1234` — `http://irobot.local`
- FQBN: `esp32:esp32:esp32`
- Gravação: `powershell -ExecutionPolicy Bypass -File tools/flash.ps1 -Sketch irobot`
- Regenerar assets: `powershell -ExecutionPolicy Bypass -File firmware/irobot/tools/generate_web_assets.ps1` (executado dentro de `firmware/irobot`)
- Comandos seriais (115200): `f` frente, `t` trás, `e` esquerda, `d` direita, `s` stop, `1`–`9` velocidade, `i` status, `r` reinicia, `a` autônomo ON/OFF, `m` ambiente + avisos
- Sensores I2C (SDA 21 / SCL 22): AHT20 `0x38` (temp/umidade), BMP280 `0x76`/`0x77` (pressão), VL53L0X `0x29` (distância)
- Modo autônomo: para a 50 cm, dá ré, gira em pivô até ter 1 m livre, detecta enroscado e desliga após 100 tentativas
- Avisos: chuva provável (tendência de pressão), calor > 26 °C, frio < 17 °C, ar seco < 30 %, possível incêndio — visíveis no painel, `/teste`, serial e `/api/sensors`
- Documentação completa: [`firmware/irobot/README.md`](firmware/irobot/README.md) e [`docs/IROBOT_NAVEGACAO_E_SENSORES.md`](docs/IROBOT_NAVEGACAO_E_SENSORES.md)


