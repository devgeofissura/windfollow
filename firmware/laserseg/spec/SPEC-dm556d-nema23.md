# Spec Delta: controladora DM556D + NEMA 23 — V1

Status: aprovada em 2026-11-19

## Objective

Trocar o conjunto de motores/driver do laserseg de "ainda não definido" para
**dois motores NEMA 23 23HS7628-TB6600 + dois drivers Azanelli DM556D**, um por
eixo (pan e tilt), usando o ramo preferencial de
`docs/example02/simcam_tracker/README.md`.

O contrato UDP v0.1 já validado (`docs/PROTOCOLO_UDP.md`) **não muda**. O
objetivo é ligar esse contrato à camada de acionamento sem regredir a
segurança: datagrama inválido, `fault() != NONE`, perda de link, fim de curso
ou watchdog devem terminar em **ambos os eixos parados e luz apagada**.

Usuário confirmado nesta sessão: motor NEMA 23, driver DM556D, **dois de cada**
para pan e tilt.

## Current behavior (baseline)

- `controller/controller.ino` : parser/estado/autoteste/telemetria, **somente
  serial**. Nenhum GPIO, motor, luz ou Wi-Fi. Gravado e validado na placa com
  `AUTOTEST total=76 passed=76 failed=0`.
- `tools/protocol_ref.py` + 84 testes em `tests/test_protocol.py`: tabela de
  decisão executável. `tools/gen_autotest_vectors.py` gera os 55 vetores C++.
- `docs/ESTADO.md`: "Motores/driver — Não definidos; bipolar + DRV8834 ou
  28BYJ-48 + ULN2003".
- Protótipos `docs/example02/simcam_tracker/` compilaram, mas **não** satisfazem
  o contrato v0.1 (`String.toInt()`, `lastPacket` atualizado antes do parse).
  Não devem ser copiados como produção.

## ASSUMPTIONS I'M MAKING

1. Motor **23HS7628-TB6600**, bipolar 2 fases / 4 fios, 1,8° (200 passos
   inteiros/volta). Corrente **2,8 A/fase** vem de ficha comercial, não da
   etiqueta do exemplar — provisória.
2. DM556D com DIP SW5–SW8 = `OFF OFF ON ON` → **3200 pulsos/volta**
   (`STEPS_PER_REV = 3200`). SW1–SW3 = `OFF OFF ON` → 2,7 A RMS / 3,8 A pico,
   próximo mas abaixo do valor provisório. SW4 (meia/plena corrente)pendente.
3. Pinagem do exemplo (`STEP/DIR` pan 4/5, tilt 6/7; limites 13/14 e 15/16;
   luz 12) é **referência do protótipo**, não pinagem confirmada da controladora
   real. Nenhum GPIO será escrito até auditoria e esquema revisado.
4. As entradas ópticas/diferenciais do DM556D exigem interface de sinal para
   lógica 3,3 V. **Não** ligar GPIO diretamente em PUL/DIR. Topologia
   comum-ânodo/comum-cátodo e corrente dos optos vêm do manual da unidade.
5. Alimentação dos motores é externa (etiqueta do driver: 24–50 Vdc), com GND
   comum conforme a interface definida; nada de alimentar motor pelo regulador
   ou USB da ESP32.
6. **SUPERADO em 2026-11-20:** LED branco difuso primeiro, com MOSFET/driver e
   resistor de gate apropriado; laser fora de escopo. A decisão vigente é laser
   acionado por módulo relé serial RoboCore (conversor 3,3→5 V, bobinas 12 V em
   fonte separada, contato NA + emergência + watchdog). Registro mantido;
   detalhes em `docs/wireviz/00-LEIA-ME.md`. Demais itens permanecem.
7. Rede do exemplo: AP `SIMCAM-SETUP`, controladora em `192.168.4.2`, UDP 4210.
   SSID/senha reais e modo AP/STA ainda precisam ser confirmados antes de
   habilitar Wi-Fi.

Corrijam qualquer item acima antes da aprovação; a spec e o código seguem
essas premissas se nada for dito.

## Tech stack / commands

- Arduino CLI 1.6.0 em `%LOCALAPPDATA%\Programs\arduino-cli\arduino-cli.exe`,
  core `esp32:esp32@3.3.11`, biblioteca `AccelStepper` (versão a fixar).
- FQBN `laserseg`:
  `esp32:esp32:esp32s3:UploadSpeed=921600,USBMode=hwcdc,CDCOnBoot=cdc,CPUFreq=240,FlashSize=16M,PartitionScheme=app3M_fat9M_16MB,PSRAM=opi`
- Python LTS **3.14.10** por caminho absoluto:
  `%LOCALAPPDATA%\Programs\Python\Python314\python.exe`
- Comandos completos:

```powershell
# testes do contrato (host)
& "$env:LOCALAPPDATA\Programs\Python\Python314\python.exe" firmware\laserseg\tests\test_protocol.py

# regerar vetores apos qualquer mudanca do contrato
& "$env:LOCALAPPDATA\Programs\Python\Python314\python.exe" `
  firmware\laserseg\tools\gen_autotest_vectors.py `
  firmware\laserseg\controller\autotest_vectors.h

# compilar (build fora do repo)
& "$env:LOCALAPPDATA\Programs\arduino-cli\arduino-cli.exe" compile `
  --fqbn "esp32:esp32:esp32s3:UploadSpeed=921600,USBMode=hwcdc,CDCOnBoot=cdc,CPUFreq=240,FlashSize=16M,PartitionScheme=app3M_fat9M_16MB,PSRAM=opi" `
  --build-path "$env:TEMP\laserseg-controller-build" `
  firmware\laserseg\controller

# deteccao antes de QUALQUER gravacao (nunca porta fixa)
powershell -ExecutionPolicy Bypass -File tools\flash.ps1 -Sketch laserseg -DetectOnly
```

## Project structure

```text
firmware/laserseg/
├── spec/SPEC-dm556d-nema23.md        este delta (proposta)
├── controller/                       parser/estado/autoteste (inalterado ate aprovacao)
├── controller_dm556d/                NOvo no de producao (a criar apos aprovacao)
│   ├── controller_dm556d.ino        nome = pasta (exigencia do arduino-cli)
│   ├── driver_config.h              pinos, passos/rev, ganho, limites
│   └── motion.h / motion.cpp        conversao erro -> alvo; parada segura
├── tools/protocol_ref.py             referencia (nao alterar contrato)
├── tools/gen_autotest_vectors.py     gerador (nao copiar tabela a mao)
└── docs/
    ├── PROTOCOLO_UDP.md              contrato v0.1 + condicoes de hardware
    ├── ESTADO.md                     decide motor/driver e pendencias
    ├── foco-luz-seguidor-001         esquema revisado de ligacao
    └── history.md                    historico compacto
```

A tabela de decisão continua **em um lugar só**. A integração deve consumir
`parser.h`/`parser.cpp` existentes; o mecanismo de compartilhamento entre
sketches (biblioteca local `src/` ou caminho comum) será definido no plano, sem
duplicar a tabela nem os vetores.

## Code style

- C++ Arduino, constantes `static const`, nomes `snake_case`, funções puras
  sempre que possível (conversão de erro em passos, limites e watchdog).
- Nenhuma lógica de protocolo dentro do `.ino`: ele só inicializa serial,
  GPIO, rede e chama `feed()` / `poll()` / camada de motion.
- Comentários em português, ASCII quando estiverem em `.ps1`.
- Nenhuma credencial no repositório; SSID/senha por `Preferences` ou define
  local fora do commit, conforme decisão a aprovar.

## Testing strategy

1. **Host (TDD):** manter os 84 testes verdes; acrescentar testes para o
   mapeamento NEMA 23 / DM556D (`STEPS_PER_REV=3200`, `MAX_TRAVEL=533`,
   `DEAD_BAND=55`, truncamento, limites 1200/1201 ms, rollover do `millis()`).
2. **Vetores:** `gen_autotest_vectors.py` regera `autotest_vectors.h`; conferir
   que a regeneração é byte a byte idêntica.
3. **Placa (somente serial, antes de GPIO):** autoteste existente 76/76 PASS.
   A nova sketch deve preservar o banner e o comando `run`.
4. **Bancada (somente após auditoria):** sem carga mecânica, fonte limitada,
   luz desligada; verificar sentido, pulso mínimo, cada fim de curso NF com
   fio desconectado, perda de Wi-Fi e watchdog.

## Boundaries

- **Always:** backup antes de editar; detectar placa antes de gravar; LED OFF
  no boot/reset/fault/link; `fault() != NONE` ⇒ parar pan + tilt + luz OFF;
  `session.poll(millis())` antes de qualquer `run()`; nunca `now_ms = -1` em
  campo; corrigir wrap-around do `millis()`; `hardware-auditor` antes de
  mexer em GPIO/alimentação/luz; `laserseg-reviewer` antes de commitar;
  `tools/save.ps1` para commit + DEVLOG + push.
- **Ask first:** atribuição de GPIOs, DIP/corrente do DM556D, SSID/senha,
  adaptação da interface 3,3 V → opto, inclusão de `AccelStepper`, envio de
  telemetria por rede, qualquer mudança do contrato UDP, uso de laser.
- **Never:** ligar GPIO 3,3 V direto em PUL/DIR; alimentar motor pela ESP32;
  copiar sketch de protótipo como produção; commitar segredos; gravar sem
  detecção e sem aprovação de hardware.

## Success criteria

1. Usuário aprovar esta spec (ou corrigir os assumptions acima).
2. 84 testes host verdes + novos testes de mapeamento NEMA 23/DM556D.
3. Vetores regenerados de forma idêntica; autoteste serial na placa PASS.
4. `controller_dm556d/` compila com `exit=0` na FQBN `laserseg`, build em
   `%TEMP%`.
5. `hardware-auditor` aprovar pinagem, interface elétrica, fontes, fins de
   curso e LED antes de qualquer gravação que toque saídas físicas.
6. Ensaio de bancada sem carga: direção, limites NF, watchdog e luz OFF
   observados; nenhum aquecimento/corrente anormal.
7. `ESTADO.md`, `PROTOCOLO_UDP.md`, `foco-luz-seguidor-001`, `README.md` e
   `history.md` atualizados; commit único via `tools/save.ps1` com push.

## Out of scope (deferred)

- `camera_node/`: nenhuma mudança de visão, sensor ou detecção nesta entrega.
- Confirmação de OV2640 vs OV5640 e do pinout da revisão da câmera.
- Telemetria `ACK` por UDP (`formatTelemetry()` já existe; transporte pendente).
- Homing automático e limites mecânicos reais além dos ±60° estimados.
- Laser, maior ganho de tracking e uso fora da bancada.
- Reorganização `docs/example/` → `example01/`/`example02/` (trabalho do
  usuário, não commitado; fora desta spec).

## Open questions

1. A etiqueta do motor confirma 1,8° e 2,8 A/fase? Qual é a base RMS/pico?
2. Posições reais dos DIP SW1–SW4 (corrente, meia/plena) e SW5–SW8?
3. Qual é o esquema/ manual exato da interface 3,3 V → PUL/DIR (comum-ânodo
   ou comum-cátodo, corrente do opto, pulso mínimo)?
4. Qual placa física será a controladora? A detectada hoje (COM4) é a
   T-SIMCAM/V1.3 usada como banco de testes; a arquitetura pede uma segunda
   ESP32-S3 separada para motores/luz.
5. Pinos reais da controladora para STEP/DIR, limites e LED no esquema revisado.
6. SSID/senha e modo (AP ou STA) da rede de bancada.
7. Existe chave de emergência / intertravamento físico?
