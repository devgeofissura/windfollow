# AGENTS.md — ventiseg

Regras locais do subprojeto `ventiseg`. A raiz `E:/dev/windfollow/AGENTS.md`
continua valendo para detecção de porta, perfis de gravação, commit/push e artefatos.

## O que é o ventiseg

Firmware para **um** motor de passo que gira o conjunto ventilador/duto,
fazendo o **pan do fluxo de ar**. A ESP32-S3 LILYGO T-SIMCAM é a única placa
do projeto: **ela detecta a pessoa com a própria câmera e comanda o pan**,
sem controladora externa. A câmera gira junto com o duto, então o laço é
fechado: `detecção na SIMCAM → erro de posição → alvo de pan → STEP/DIR no
DM556D → motor → duto e câmera giram juntos → novo quadro`. A visão precisa
compensar o próprio movimento para manter a pessoa centralizada e o fluxo de
ar apontado a ela.
Não há laser, luz, módulo relé, fim de curso nem eixo de tilt.

## Regra de ouro

> Enquanto `VENTISEG_ENABLE_GPIO=0`, o firmware é **serial-only**: ele valida
> protocolo, estado, watchdog e telemetria, mas **não escreve em GPIO algum**.
> Nenhuma gravação com GPIO ativo sem auditoria elétrica fechada.

Ordem obrigatória: `tools/protocol_ref.py` → `controller/parser.{h,cpp}` →
`tools/gen_autotest_vectors.py` → `controller/autotest.h` → `simulator/`.
Qualquer mudança de protocolo atualiza `docs/PROTOCOLO.md`, `docs/ESTADO.md`
e `docs/DEVLOG.md` da raiz no mesmo commit.

## Pinagem confirmada pelo usuário (J_GPIO_LIVRES, conector preto 6 pinos)

| Pino | Sinal | Uso no ventiseg | Observação |
|---|---|---|---|
| 1 | IO3 / GPIO3 | `STEP` (PUL) | Digital/PWM/ADC; assumimos ADC de bateria **não usado** |
| 2 | IO46 / GPIO46 | livre | **STRAPPING**: não puxar HIGH no boot |
| 3 | IO45 / GPIO45 | livre | **STRAPPING**: define tensão da Flash no boot |
| 4 | IO21 / GPIO21 | `DIR` | Mais seguro para sinal; digital/PWM/I2C |
| 5 | GND | referência | Comum com o driver |
| 6 | 3V3 | referência | Sensores apenas; **não aplicar 5 V nos sinais** |

IO45 e IO46 ficam reservados. A lógica do driver deve ser 3,3 V ou
interfaceada por conversor — `NAO CONFIRMADO` até conferir o manual/foto do
DM556D Azaneli.

## Antes de qualquer gravação

1. Detectar porta na hora: `powershell -ExecutionPolicy Bypass -File tools/flash.ps1 -Sketch ventiseg -DetectOnly`.
2. O script confere chip ESP32-S3 e o perfil `ventiseg` (FQBN S3, 16 MB,
   `app3M_fat9M_16MB`, PSRAM OPI).
3. Compilar em `$env:TEMP`; nunca criar build dentro do repositório.
4. Com `VENTISEG_ENABLE_GPIO=0`, gravar é seguro para bancada de lógica.
5. Para habilitar motor: fechar P1–P5 em `docs/ESTADO.md` e registrar decisão.

## Convenções

- Sketch `.ino` com o mesmo nome da pasta: `controller/controller.ino`.
- Constantes C++ `k...`, macros `VENTISEG_*`, Python `MAIÚSCULAS`, JS `SIM.*`.
- Testes host: `powershell -ExecutionPolicy Bypass -File firmware/ventiseg/tests/run.ps1`.
- Um commit por tarefa, com `tools/save.ps1`.
