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

## NAO CONFIRMADO (não montar sem verificar)

- Corrente do LED dos optos e pulso mínimo aceitável no **exemplar** do DM556D.
- Origem do **5 V** do conversor (regulador 24 V→5 V ou saída auxiliar do driver).
- Posições exatas de **SW5–SW8** para 1/8 de passo: conferir a tabela impressa
  na carcaça. O firmware mantém 3200 pulsos/volta provisórios; 1600 é o alvo.
- Tensão/potência exata da fonte 24 V, fusível, bitolas e dissipação.
- Acoplamento mecânico motor↔ventilador, curso útil em passos e batentes.
- Montagem mecânica da SIMCAM no conjunto girante: suporte, folga do cabo de
  alimentação/serial e raio de rotação sem trancar ou puxar conector.
- Referência/home do eixo: sem sensor, a posição inicial é convencional.
- Modelo de detecção da SIMCAM (biblioteca, resolução, FPS, limiar) e escala
  entre `x ∈ [-1000,1000]` e graus de pan: `NAO CONFIRMADO`.
- Revisão física da T-SIMCAM (V1.3 microfone I2S x V1.4 PDM) — irrelevante
  para STEP/DIR, mas registrada para não virar "fato".
- Comportamento dos pinos durante boot/reset do driver (pulso falso no motor).

## Contradições conhecidas

A skill `espressif-lilygo` e o YAML `no-visao-s3-camsim.yaml` do laserseg
listam IO45/46 como RX/TX do modem e IO21 como status/LED. A serigrafia do
conector fornecida pelo usuário diz que são os **únicos GPIOs livres**.
Para o ventiseg prevalece a serigrafia do usuário (fonte primária desta
bancada), com IO45/46 reservados por strapping e IO21 em uso como DIR.
A divergência permanece registrada, não "resolvida por suposição".

Também foi registrada a inversão IO3↔IO21 que apareceu na especificação do
driver; a decisão explícita do usuário foi manter a serigrafia.
