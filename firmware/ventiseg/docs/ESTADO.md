# Estado do ventiseg

Atualizado em 21/11/2026.

## Fase atual

**Bancada de lógica — sem energizar motor.** O firmware é serial-only por
construção (`VENTISEG_ENABLE_GPIO=0`). Nenhuma gravação com GPIO ativo foi
feita. A auditoria P1–P5 está documentada, mas **fisicamente aberta**.

## Confirmado nesta entrega

- Perfil `ventiseg` registrado no `tools/flash.ps1` e na tabela de perfis.
- Pinagem STEP=IO3, DIR=IO21, IO45/46 reservados por strapping.
- Driver DM556D (Azaneli/Policomp/Tecmaf), 24–50 VDC, 2,1–5,6 A, alimentado
  a 24–36 VDC / mín. 4 A.
- Motor **KTC-HT23-400**, 8 fios, bipolar paralela 2,8 A, 1,9 Nm; fios
  mapeados para `A±/B±` e DIP SW1–SW4 definidos.
- Interface em **catodo comum** com conversor obrigatório **3,3 V → 5 V**
  (optos exigem alto > 3,5 V); `ENA` desconectado, driver inicia habilitado.
- A **SIMCAM detecta a pessoa e comanda o pan** — visão e controle na mesma
  placa, sem controladora externa (reafirmado pelo usuário em 20/11/2026).
- **A câmera gira junto com o duto**: malha fechada; a visão compensa o
  próprio movimento do pan.
- Protocolo v0.1 especificado; `T` é o resultado da detecção interna.
  Porta UDP 4211 apenas reservada para depuração futura; serial-only agora.
- Guia executivo de ligação em `docs/LIGACOES.md` e esquema WireViz em nível
  de terminal (`PUL±/DIR±/ENA±`, fases `A±/B±`, conversor 3,3→5 V, 24 V):
  **variante catodo comum desenhada**, validada pelo WireViz 0.4.1.
- Núcleo TDD entregue: `tools/protocol_ref.py` → `controller/parser.cpp` →
  52 vetores gerados → `autotest.h` → simulador, com **149 testes host
  verdes** (113 protocolo + 28 simulador + 4 diferenciais de parser + 4
  diferenciais de sessão). Watchdog renovado **somente** por `T` aceito no
  firmware e no simulador (divergência corrigida).
- Comportamento seguro pós-`Lnk` definido e aplicado nas três implementações:
  após o watchdog, `TRACK`/`MANUAL` são inibidos, `Lnk` domina `Range` e só um
  `T` aceito restaura o link — ainda em SAFE até um novo comando de movimento.
  Contrato em `PROTOCOLO.md`, verificação em `tests/test_safe_state.py`.
- Testes diferenciais Python × JavaScript × C++: 60 datagramas (52 vetores do
  autoteste + 8 casos de divergência) alimentam as três implementações e
  comparam `kind`, `reason`, `accepted`, `seq`, `x`, `valid`, `enabled` e
  `direction`. O harness C++ compila `controller/parser.cpp` no host e o
  resultado só é considerado verde quando as três assinaturas batem; sem
  compilador host o teste é pulado com aviso (o autoteste em placa cobre o
  C++). Defina `VENTISEG_CXX` para apontar para `g++`, `clang++`, `c++` ou
  `zig.exe` (que usa o subcomando `c++`).
- Divergências do simulador corrigidas em 21/11/2026 para espelhar Python/C++:
  teto de `seq` = `INT32_MAX` (antes aceitava até 2^53), idade `rx_ms`/`now_ms`
  aplicada a qualquer comando (antes inexistente), zeros à esquerda aceitos e
  rejeição por faixa ligando `fault=Range` (antes só por injeção na UI).
  `PROTOCOLO.md` e o painel do simulador agora documentam a ordem completa
  `tamanho/codificação → formato → faixa → sequência → idade`.
- Sketch compila na FQBN `ventiseg` (315.392 B no binário gravado), com
  `VENTISEG_ENABLE_GPIO=0` e nenhuma chamada de GPIO/Wi-Fi.
- Autoteste validado na placa real em 21/11/2026: porta detectada na hora
  (COM4 · ESP32-S3 · flash 16 MB · MAC `dc:da:0c:28:25:30`), upload com hash
  verificado e boot capturado sem terminal. Log: `gpio_gate=0`,
  `AUTOTEST total=85 passed=85 failed=0`, `AUTOTEST PASS`, `ACK,-1,SAFE,None`
  e heartbeat. Não existe chamada `pinMode`/`digitalWrite`/`ledc` no
  controller: a placa permaneceu serial-only.

## Ainda não feito

- `VENTISEG_STEPS_PER_REV` segue **3200 (provisório)**. O alvo é 1600
  (1/8 passo) assim que SW5–SW8 forem lidos na carcaça; a troca exige
  regenerar vetores e rodar a suíte no mesmo commit.

## Pendências antes de qualquer ensaio com motor

| ID | Pendência | Situação em 21/11/2026 | Bloqueia |
|---|---|---|---|
| P1 | Corrente do opto, pulso mínimo e origem do 5 V | Documentação: 8–20 mA (típ. 10), entrada 5–24 V, resistor 2,2 kΩ; **5 V externo obrigatório**. Etiqueta, conversor e 5 V medido: pendentes | Energização do sinal |
| P2 | SW5–SW8 e curso útil/batentes | Tabela conhecida: 1600 = `ON/OFF/ON/ON` (firmware segue 3200). Curso, batentes e acoplamento: não medidos; 533 é provisório | Movimento |
| P3 | Fonte, fusível, bitolas e aterramento | Proposta: 24 V/6,25 A/150 W, fusível DC 7,5 A retardado, 1,5 mm² potência, 1,0–1,5 mm² motor, 0,22–0,34 mm² sinal blindado, star ground. Componentes reais: não conferidos | Energização |
| P4 | Acoplamento e limite por software | Sem acoplamento, home, batentes ou curso medidos; nenhum `kMaxTravel` validado | Movimento |
| P5 | Pulso no boot/reset e travamento com carga desacoplada | Somente verificação lógica (`gpio_gate=0`, autoteste 85/85); sem medição elétrica | Habilitação de saída |

Nenhuma P foi fechada fisicamente. Não habilitar GPIO, não energizar
driver/motor e não avançar para a camada de saída STEP/DIR até P1–P5 serem
conferidas no equipamento real. Detalhes em `HARDWARE-DECISIONS.md`.

## Regra de atualização

Toda mudança de firmware atualiza este arquivo, `docs/PROTOCOLO.md`,
`docs/HARDWARE-DECISIONS.md` e `docs/DEVLOG.md` da raiz no mesmo commit.
