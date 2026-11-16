# iRobot — Firmware ESP32 (Robô Diferencial de 2 Rodas + Caster)

Projeto baseado no hardware existente do **WindFollowBlind**, adaptado para um chassi diferencial de 2 motores:
- **2 motores DC com redutor**, cada um acionando **uma roda** (esquerda/direita).
- **1 roda caster livre** (giratória) na outra ponta do chassi, apenas de apoio.
- **Driver L298N** com controle de PWM nos pinos ENA/ENB.
- **ESP32 DevKit** com Access Point, mDNS e interface web embarcada.
- **Sem botões físicos** — controle exclusivamente via interface web / API REST.

## Hardware

Chassi diferencial: **roda motriz esquerda + roda motriz direita** na traseira e **roda caster livre** (giratória, 360°) na dianteira (ou vice-versa), apenas como ponto de apoio.

| Sinal | GPIO | Função |
|-------|------|--------|
| Motor Esquerdo IN1 | 26 | Sentido 1 da roda esquerda |
| Motor Esquerdo IN2 | 25 | Sentido 2 da roda esquerda |
| Motor Esquerdo ENA (PWM) | 27 | Habilitação + velocidade |
| Motor Direito IN3 | 32 | Sentido 1 da roda direita |
| Motor Direito IN4 | 33 | Sentido 2 da roda direita |
| Motor Direito ENB (PWM) | 14 | Habilitação + velocidade |
| Roda caster | — | Livre, sem atuador nem sensor |

**Regras elétricas:**
- Fonte separada para os motores (nunca pelos 3V3/5V do ESP32).
- GND comum entre ESP32, L298N e fonte dos motores.
- Jumpers ENA/ENB do módulo L298N devem ser **removidos** para que o PWM funcione.

**Alimentação sem USB (ligação validada em bancada):**
- Bateria **7–12 V** nos terminais `+12V` e `GND` do L298N, com o jumper `5V-EN` no lugar.
- Saída **`5V` do L298N → pino `VIN` (ou `5V`) do ESP32**.
- **`GND` do L298N → `GND` do ESP32** (mesma referência, obrigatório).
- Não ligar o USB junto com essa alimentação no `VIN` (backfeed).

## Interface Web

Acesse via `http://irobot.local` ou IP do Access Point (`192.168.4.1` por padrão).
Versão minimalista só com 5 botões (frente/trás/esquerda/direita/parar): `http://192.168.4.1/teste`.

### Controles
- **Frente / Tras / Esquerda / Direita / Parar**: movimento diferencial do robô.
- **Velocidade Esquerda / Direita**: sliders independentes (0–255) para curvas suaves.
- **Aplicar Velocidades**: envia controle diferencial direto para cada motor.
- **Configuracao**: PWM minimo/maximo/padrao, salvo em NVS.

### API REST

| Endpoint | Metodo | Descricao |
|----------|--------|-----------|
| `/api/motor/status` | GET | Status dos motores (direcao, velocidades, movendo) |
| `/api/motor/move` | POST | Move o robô (`direction`: frente/tras/esquerda/direita/stop, `speed`: 0-255) ou diferencial (`leftSpeed`, `rightSpeed`) |
| `/api/motor/stop` | POST | Para todos os motores |
| `/api/config` | GET/POST | Ler/salvar configuracao de PWM |
| `/api/system/info` | GET | Uptime, heap, PSRAM |

## Compilacao

A porta COM muda a cada replug do cabo, entao o script detecta a porta e confere o
chip (ESP32 classico) na hora:

```text
powershell -ExecutionPolicy Bypass -File tools/flash.ps1 -Sketch irobot
```

Para apenas listar as portas e o chip de cada uma:

```text
powershell -ExecutionPolicy Bypass -File tools/flash.ps1 -Sketch irobot -DetectOnly
```

Antes de compilar apos editar `web/` (rodar dentro de `firmware/irobot`):

```text
powershell -ExecutionPolicy Bypass -File tools/generate_web_assets.ps1
```

## Comandos Seriais (115200 baud)

| Comando | Efeito |
|---------|--------|
| `f` | Frente |
| `t` | Tras |
| `e` | Esquerda |
| `d` | Direita |
| `s` | Stop |
| `1`–`9` | Velocidade frente (10%–90%) |
| `i` | Status |
| `r` | Reiniciar |

## Arquivos

| Arquivo | Funcao |
|---------|--------|
| `irobot.ino` | Setup/loop, AP + mDNS, comandos seriais |
| `HardwareConfig.h` | Struct de configuracao (GPIOs, limites PWM) |
| `L298NDualDriver.h/.cpp` | Controle do L298N com PWM |
| `MotorEngine.h/.cpp` | Comandos diferenciais do robô |
| `ConfigManager.h/.cpp` | Leitura/gravacao em NVS |
| `DashboardService.h/.cpp` | Servidor HTTP, API REST |
| `WebAssets.h` | HTML/CSS/JS embarcados |
| `web/` | Fontes da interface web |
| `tools/generate_web_assets.ps1` | Gera WebAssets.h a partir de web/ |

## Protótipos

### Protótipo-67

- Montagem de bancada do projeto irobot.
- Alimentado com **2 baterias 3,7 V em série (7,4 V)** no L298N, que também
  alimentou o ESP32 pela saída 5 V (`VIN`).
- Deu conta de alimentar ESP32 + drivers + motores, mas ficou **um pouco fraco**
  (7,4 V é o mínimo do L298N, que derruba ~2 V: sobram ~5 V nos motores).
- Mesmo assim, **todas as funcionalidades foram testadas** (web em `/` e
  `/teste`, comandos, PWM e AP Wi-Fi).
- Próximo passo de potência: 3 células (11,1 V) ou pack 12 V com buck para o
  ESP32, mantendo o GND comum.

## Limites Conhecidos

- Sem encoders → controle em malha aberta (nao sabe distancia/angulo exato).
- Motores tipo TT → baixo torque, adequado para piso plano e robô leve.
- A roda caster absorve a deriva lateral nas curvas, mas exige piso liso: em carpete ou piso irregular ela trava e o robô desvia.
- Só duas rodas motrizes → menor tração que um chassi de 4 rodas; manter o centro de gravidade próximo do eixo motriz.
- `e`/`d` giram em torno da roda parada (uma roda para frente, a outra travada). Girar no próprio eixo exigiria uma roda para frente e a outra para trás.

## Sensores ambientais e avisos

| Sensor | Endereco | Uso |
|--------|----------|-----|
| AHT20 | I2C `0x38` | temperatura e umidade |
| BMP280 | I2C `0x76`/`0x77` | pressao (tendencia do tempo) |
| VL53L0X | I2C `0x29` | distancia (desvio autonomo) |

`GET /api/sensors` devolve tambem o objeto `alerts`:

```json
"alerts": { "rain": false, "rainSensor": false, "hot": false, "cold": false,
            "dry": false, "fire": false, "trendReady": true,
            "trend3h": -0.4, "trend30m": -0.1, "tempRise": 0.2 }
```

### Regras (limiares em `HardwareConfig.h`, bloco `ENV_*`)

- **Vai chover**: pressao caiu >= 1,0 hPa em 3 h e umidade >= 65 %, ou queda
  >= 0,6 hPa em 30 min com umidade >= 70 %. A previsao so aparece depois de
  30 min de historico (regra classica de barometro).
- **Chuva agora**: sensor FC-37/YL-83 molhado (opcional).
- **Calor**: acima de 26 C -> avisa para ligar o ar-condicionado.
- **Frio**: abaixo de 17 C -> avisa para desligar o ar-condicionado.
- **Ar seco**: umidade abaixo de 30 % -> avisa para beber agua.
- **Incendio**: temperatura >= 45 C, subida >= 4 C em 1 min ou sensor de chama
  KY-026 (opcional). O app mostra banner vermelho, vibra e apita.

### Sensores opcionais

Ainda nao ligados no Prototipo-67. Para habilitar, descomente em
`HardwareConfig.h`:

```c
#define IROBOT_RAIN_SENSOR_PIN   34   // FC-37/YL-83 AO (ADC1, entrada)
#define IROBOT_FLAME_SENSOR_PIN  39   // KY-026 AO (ADC1, entrada)
#define IROBOT_RAIN_WET_ADC      1800 // menor = mais molhado
#define IROBOT_FLAME_ADC         1000 // menor = chama presente
```

Alimente os modulos em 3,3 V e use apenas pinos ADC1 (GPIO 32-39): o ADC2 nao
funciona com o Wi-Fi ligado.
