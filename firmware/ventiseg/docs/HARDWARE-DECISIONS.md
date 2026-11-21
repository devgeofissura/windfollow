# Decisões de hardware — ventiseg

Fonte primária: transcrição da serigrafia e decisões do usuário em 20/11/2026,
complementadas pela especificação do driver/motor enviada pelo usuário na mesma
data. Tudo que não está em **Confirmado** é `NAO CONFIRMADO` e não autoriza
montagem.

## Confirmado

- A placa disponível é a **ESP32-S3 LILYGO T-SIMCAM**, 16 MB de flash,
  detectada em COM4 com MAC `dc:da:0c:28:25:30` (porta muda; sempre redetectar).
- Conector preto `J_GPIO_LIVRES`, 6 pinos:
  `1 IO3`, `2 IO46`, `3 IO45`, `4 IO21`, `5 GND`, `6 3V3`.
- IO45 e IO46 são pinos de **strapping**: não puxar HIGH no boot (IO46) e
  cuidado com a tensão da Flash no boot (IO45). Por isso ficam reservados.
- **STEP = IO3**, **DIR = IO21**. IO3 é ADC de bateria na SIMCAM; como não
  há bateria no projeto, foi liberado para digital/PWM. Uma mensagem posterior
  inverteu os dois sinais; foi decidido em 20/11/2026 que aquilo era **typo** e
  a serigrafia prevalece.
- Nível lógico dos sinais da placa é **3,3 V**. Não aplicar 5 V nos pinos.
- Driver é **DM556D** (Azaneli / Policomp / Tecmaf), faixa **24–50 VDC** e
  corrente **2,1–5,6 A**. Alimentação escolhida: **24–36 VDC, mínimo 4 A**.
- Interface de sinal: **catodo comum** — GND da SIMCAM em `PUL-`/`DIR-`, sinais
  de **5 V** em `PUL+`/`DIR+`. Os optos exigem nível alto `> 3,5 V`, então um
  **conversor de nível bidirecional 3,3 V ↔ 5 V é obrigatório**; nunca ligar o
  GPIO direto.
- `ENA+`/`ENA-` ficam **desconectados**: o driver já inicia habilitado por padrão.
- Motor **KTC-HT23-400**, 8 fios, ligação **bipolar paralela** a **2,8 A**,
  torque **1,9 Nm**. Fios: `A+`=Vermelho+Preto/Branco, `A-`=Preto+Vermelho/Branco,
  `B+`=Verde+Amarelo/Branco, `B-`=Amarelo+Verde/Branco.
- DIP do driver: SW1/SW2/SW3 = `OFF/ON/OFF` (2,7–2,8 A) e SW4 = `OFF` (meia
  corrente em repouso). Alvo de micropasso: **1/8 → 1600 pulsos/volta**.
- A **SIMCAM detecta a pessoa e comanda o pan**: a visão roda na própria
  ESP32-S3, sem câmera externa, sem controladora separada e sem PC no laço.
  O erro de posição detectado vira o alvo do eixo único.
- **A câmera gira junto com o duto/ventilador** (decisão do usuário em
  20/11/2026): placa, câmera e saída de ar formam um único conjunto móvel
  acionado pelo motor. É uma **malha fechada** — quando o pan gira, o quadro
  gira junto e a detecção precisa compensar esse movimento. O objetivo é manter
  a pessoa centralizada no quadro **e** o fluxo de ar apontado a ela.
- Um único motor de passo aciona esse conjunto, via DM556D.
- Não existem fim de curso, módulo relé, laser nem luz neste projeto.

## Documentação de referência consultada em 21/11/2026

Isso **não** substitui a etiqueta/revisão do exemplar instalado nem medição:

- Policomp DM556D: 24–50 VDC, 2,1–5,6 A, 16 micropassos, entrada de comando
  5–24 VDC e resistor série de **2,2 kΩ** indicado pelo fabricante.
- Tecmaf TMA.DMP.556D: nível alto lógico **4–5 V**, baixo **0–0,5 V**,
  alimentação 24–50 VDC. O manual **não** documenta terminal auxiliar de 5 V;
  portanto o 5 V do conversor de nível deve ser **externo**.
- Corrente dos optos `PUL`/`DIR` na documentação do DM556D consultada:
  **8 mA mínimo / 10 mA típico / 20 mA máximo por entrada**. Pior caso com
  `PUL` e `DIR` ativos: ~40 mA. O manual do DM556 original (Leadshine) dá
  7/10/16 mA; não transferir esse número entre variantes. Projeto usa
  ~10 mA/canal e faixa 8–20 mA até conferir o exemplar.
- Micropasso (tabelas Policomp e Tecmaf): **1600 pulsos/volta = SW5 ON,
  SW6 OFF, SW7 ON, SW8 ON**; 3200 = `OFF/OFF/ON/ON`. O firmware segue 3200
  provisório; trocar exige regenerar vetores e rodar a suíte no mesmo commit.

## NAO CONFIRMADO (não montar sem verificar)

- Corrente do LED dos optos e pulso mínimo aceitável no **exemplar** do DM556D
  (documentação de referência: 8–20 mA, típ. 10 mA; valor real pendente).
- Origem do **5 V** do conversor: nenhuma saída auxiliar documentada; DC-DC
  externo 24 V→5 V é a proposta, mas conversor e tensão medida pendem.
- Posições reais de **SW5–SW8** no exemplar: a tabela é conhecida
  (1600 = `ON/OFF/ON/ON`), mas falta ler a carcaça. Firmware segue 3200.
- Componentes reais de fonte/fusível/bitolas/dissipação (proposta em P3).
- Acoplamento mecânico motor↔ventilador, curso útil em passos e batentes.
- Montagem mecânica da SIMCAM no conjunto girante: suporte, folga do cabo de
  alimentação/serial e raio de rotação sem trancar ou puxar conector.
- Referência/home do eixo: sem sensor, a posição inicial é convencional.
- Modelo de detecção da SIMCAM (biblioteca, resolução, FPS, limiar) e escala
  entre `x ∈ [-1000,1000]` e graus de pan: `NAO CONFIRMADO`.
- Revisão física da T-SIMCAM (V1.3 microfone I2S x V1.4 PDM) — irrelevante
  para STEP/DIR, mas registrada para não virar "fato".
- Comportamento **elétrico** dos pinos durante boot/reset do driver: apenas
  verificação lógica foi feita, sem osciloscópio nem carga desacoplada.

## Auditoria P1–P5 — status em 21/11/2026

| ID | Situação | Evidência / próximo passo |
|---|---|---|
| P1 | **Aberta** | Documentação de referência fecha corrente (8–20 mA, típ. 10 mA), entrada 5–24 V e necessidade de **DC-DC externo 24 V → 5 V** (≥100 mA, margem de 200 mA). Faltam foto da etiqueta/revisão do DM556D, datasheet do conversor 3,3↔5 V instalado, capacidade do DC-DC e **5 V medido antes de ligar PUL+/DIR+**. |
| P2 | **Parcial** | Tabela de micropasso conhecida (1600 = `ON/OFF/ON/ON`), mas posição real no exemplar não foi lida. Curso útil, batentes e acoplamento **não medidos**. `kMaxTravel=533` é provisório; com 1600 pulsos/volta, 533 ≈ 120° por lado e 267 ≈ 60° apenas como hipótese 1:1. Não adotar nenhum sem medição. |
| P3 | **Aberta** | Proposta de dimensionamento: fonte 24 VDC regulada **6,25 A / 150 W** exclusiva, fusível DC **7,5 A retardado**, 1,5 mm² fonte→driver, 1,0–1,5 mm² driver→motor, par trançado blindado 0,22–0,34 mm² para sinal e **star ground** único (PE só no chassi). Valores **não conferidos**: comprimentos, corrente real da fonte, curva do fusível, queda de tensão, dissipação e continuidade do terra. |
| P4 | **Aberta** | Sem acoplamento definido, sem referência/home, sem batentes documentados e sem curso medido. Limite por software só depois de medir a distância do home ao batente com margem de segurança. Nenhum `kMaxTravel` validado. |
| P5 | **Aberta** | "Sem pulso e trava ok" existiu apenas no **nível lógico** (`gpio_gate=0`, autoteste 85/85, nenhuma chamada de GPIO). Não houve osciloscópio em `PUL+`/`DIR+`, nem ensaio com motor desacoplado, nem observação de boot/reset do driver. |

Consequência: **nenhuma P foi fechada fisicamente**. Enquanto isso, o firmware
continua em `VENTISEG_ENABLE_GPIO=0`, serial-only, sem STEP/DIR, sem motor
energizado e sem câmera ativa.

## Contradições conhecidas

A skill `espressif-lilygo` e o YAML `no-visao-s3-camsim.yaml` do laserseg
listam IO45/46 como RX/TX do modem e IO21 como status/LED. A serigrafia do
conector fornecida pelo usuário diz que são os **únicos GPIOs livres**.
Para o ventiseg prevalece a serigrafia do usuário (fonte primária desta
bancada), com IO45/46 reservados por strapping e IO21 em uso como DIR.
A divergência permanece registrada, não "resolvida por suposição".

Também foi registrada a inversão IO3↔IO21 que apareceu na especificação do
driver; a decisão explícita do usuário foi manter a serigrafia.
