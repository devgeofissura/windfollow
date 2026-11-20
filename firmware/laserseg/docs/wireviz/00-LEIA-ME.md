# Documentação de hardware em WireViz — laserseg

Conjunto versionável que responde às três perguntas feitas para o futuro
projeto de PCB/shield:

| Pergunta | Arquivo |
|---|---|
| Esquemático elétrico + diagrama de ligação do nó controladora | [`controladora-dm556d-nema23.yaml`](controladora-dm556d-nema23.yaml) |
| Mapa de pinagem/GPIOs da controladora | [`pinout-controladora-s3.yaml`](pinout-controladora-s3.yaml) |
| Pinout da placa de câmera (GPIOs livres e ocupados) | [`no-visao-s3-camsim.yaml`](no-visao-s3-camsim.yaml) |

O WireViz gera **schematic e wiring diagram a partir do mesmo YAML**. O
terceiro arquivo é um mapa de pinos onboard (a câmera já vem ligada na placa;
não há cabo novo a adicionar nesse nó).

## Como renderizar

```powershell
wireviz firmware\laserseg\docs\wireviz\controladora-dm556d-nema23.yaml
wireviz firmware\laserseg\docs\wireviz\pinout-controladora-s3.yaml
wireviz firmware\laserseg\docs\wireviz\no-visao-s3-camsim.yaml
```

Requer `wireviz` e o Graphviz (`dot`) no PATH para SVG/PNG. Nesta máquina o
parser WireViz **0.4.1** validou os três arquivos (`w.parse(...)` com
`return_types='harness'`); o Graphviz não está instalado, então a renderização
gráfica ainda não foi gerada.

## Arquitetura — não inverter

```text
S3_CAM_SIM (câmera já ligada na placa, POUCOS GPIOs livres)
        │  UDP 4210 — só posição/erro
        ▼
ESP32-S3 controladora SEPARADA → 2x DM556D → 2x NEMA 23
                               → 4 fins de curso NF
                               → conversor 3,3→5 V → módulo relé serial
                                 → contato NA → LASER
```

A SIMCAM **não** aciona motores, fins de curso nem iluminador. Ela tem apenas
quatro GPIOs livres, lógica de 3,3 V e pinos de strapping no boot. Motores e
luz exigem driver e fonte externa separada. Isso está registrado em
`docs/foco-luz-seguidor-001`, `docs/HARDWARE-DECISIONS.md` e
`docs/base-de-conhecimento-rastreamento-com-s3-cam-sim.md`.

### Saída de luz — mudança em 2026-11-20

O iluminador passou de **LED difuso + MOSFET** para **laser acionado por módulo
rele serial RoboCore** (Clock/Data com **5 VDC**, bobinas **12 VDC / 30 mA** por
relé, até 10 módulos em cascata). O esquema antigo (`GPIO12/LED_EN`) está
marcado como **SUPERADO** nos YAMLs, mas mantido como registro histórico.

- A controladora é 3,3 V: **conversor de nível 3,3 → 5 V obrigatório** nos dois
  pinos de controle. Não ligar GPIO direto no módulo. O lado HV precisa de um
  rail de **5 V regulado próprio** (origem/corrente ainda não confirmados),
  nunca dos 12 V das bobinas.
- Fontes separadas: 12 V exclusivo para as bobinas, fonte/driver próprio para o
  laser e 5 V regulado para o conversor. Todos os negativos convergem em um
  **ponto único de referência (`J_STAR_GND` no YAML)**; sem isso Clock/Data
  ficam sem referência comum.
- Intertravamento definido: **contato NA** (desenergizado = laser OFF), chave
  física de emergência e watchdog de link. Laser OFF no boot, reset, perda de
  link e watchdog.
- GPIOs da controladora para Clock/Data: **AINDA NÃO CONFIRMADOS**.
- Fonte dos dados: <https://github.com/RoboCore/SerialRelay> (confirmado pelo
  usuário em 2026-11-20).

## Pinout da S3_CAM_SIM — informado pelo usuário em 2026-11-20

Fonte primária: serigrafia da placa e mapeamento interno detalhados pelo
usuário, transcritos para `no-visao-s3-camsim.yaml`.

### GPIOs realmente livres (conector preto, 6 pinos)

| Serigrafia | ESP32-S3 | Uso recomendado / ressalva |
|---|---|---|
| IO3 | GPIO 3 | Digital, PWM ou ADC analógico |
| IO46 | GPIO 46 | Saída digital/PWM; **strapping** — evitar nível alto no boot |
| IO45 | GPIO 45 | Digital/PWM; **strapping** — define tensão da Flash no boot |
| IO21 | GPIO 21 | Digital, PWM, I2C (SDA/SCL) — o mais seguro para sensores |
| GND | GND | Terra |
| 3V3 | 3,3 V | Alimentação de sensores |

### Conector branco JST 4 pinos (UART/Grove, ao lado da USB-C)

| Pino | ESP32-S3 | Uso |
|---|---|---|
| GND | — | Terra |
| 5V/VCC | — | Alimentação |
| U0TXD | GPIO 43 | TX serial |
| U0RXD | GPIO 44 | RX serial |

TX/RX podem virar GPIOs extras **somente** se o monitor serial não estiver em
uso.

### Pinos já ocupados internamente

| Periférico | GPIOs |
|---|---|
| Câmera OV2640 | SIOD/SDA 4, SIOC/SCL 5, VSYNC 6, HREF 7, PCLK 13, XCLK 14, D0..D7 = 11, 9, 8, 10, 12, 17, 16, 15, RESET 18 |
| Microfone I2S | SCK 41, WS 42, DOUT 2 |
| microSD (SPI) | CS 47, MOSI 38, CLK 39, MISO 40 |
| mPCIe (modem/SIM) | TX 48, RX 35, PWR/RST 1, 20 ou 46 (depende da revisão) |

**Elétrica:** não aplicar 5 V nos pinos de sinal; a lógica é de 3,3 V.

### Divergências registradas (não resolvidas por suposição)

| Item | Usuário | Tabela oficial / issue #33 |
|---|---|---|
| RESET da câmera | GPIO 18 | oficial: não usado (−1), GPIO 18 = IR-CUT |
| RX do modem | GPIO 35 | oficial: 46; issue: 45/46 |
| GPIO 21 | I2C/sensor seguro | oficial: STATUS do modem; issue: LED mPCIe |
| GPIO 45/46 | livres (com strapping) | issue: UART do modem |
| Sensor | OV2640 | projeto ainda registra OV2640/OV5640 até leitura de `sensor_id` |
| Revisão da placa | — | não confirmada em bancada |

Antes de transformar isso em shield/PCB: conferir esquema da revisão,
serigrafia e medir sem alimentação. Nenhuma divergência deve ser "chutada".

## Pinout da controladora (V0 — protótipo)

Valores extraídos de `docs/example02/`, **não validados em bancada**:

| Sinal | GPIO |
|---|---|
| STEP/DIR pan | 4 / 5 |
| STEP/DIR tilt | 6 / 7 |
| Fins de curso NF pan | 13 / 14 |
| Fins de curso NF tilt | 15 / 16 |
| Relé serial Clock/Data | **não definido** — conversor 3,3 → 5 V |
| Foco LED (antigo) | 12 — **SUPERADO** pelo laser via relé |
| ENA | não definido — não ligar sem o manual do DM556D |

A interface 3,3 V → opto do DM556D é obrigatória; sua topologia
(comum-ânodo/catodo, corrente e pulso mínimo) ainda não foi confirmada.

O mapa antigo de LED no GPIO 16 e limites nos GPIOs 17/18 (comum em
`docs/foco-luz-seguidor-001`) também está **SUPERADO** pela referência V0 do
`example02` adotada aqui. Nenhum desses números deve ser usado em PCB sem
revisão da placa controladora escolhida.

## Estado da documentação

- [x] Três artefatos WireViz escritos e validados pelo parser 0.4.1.
- [x] Pinout detalhado da SIMCAM incorporado como fonte primária.
- [x] Divergências marcadas em vez de silenciadas.
- [x] Saída de luz migrada para laser + relé serial; conversor 3,3→5 V e
      fonte 12 V separada registrados como obrigatórios.
- [x] Aba **Pinout** (somente leitura) no simulador espelha o YAML da visão.
- [ ] Renderização SVG/PNG (depende de instalar Graphviz).
- [ ] Revisão final do `hardware-auditor` antes de qualquer `GPIO=1`.
- [ ] Sensor, revisão e interface elétrica confirmados em bancada.
- [ ] GPIOs Clock/Data do relé escolhidos e validados em bancada.

Esta é documentação: nenhum GPIO, motor, luz ou firmware foi alterado.
