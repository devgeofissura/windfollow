# Decisões de hardware — ventiseg

Fonte primária: transcrição da serigrafia e decisões do usuário em 20/11/2026.
Tudo que não está em **Confirmado** é `NAO CONFIRMADO` e não autoriza montagem.

## Confirmado

- A placa disponível é a **ESP32-S3 LILYGO T-SIMCAM**, 16 MB de flash,
  detectada em COM4 com MAC `dc:da:0c:28:25:30` (porta muda; sempre redetectar).
- Conector preto `J_GPIO_LIVRES`, 6 pinos:
  `1 IO3`, `2 IO46`, `3 IO45`, `4 IO21`, `5 GND`, `6 3V3`.
- IO45 e IO46 são pinos de **strapping**: não puxar HIGH no boot (IO46) e
  cuidado com a tensão da Flash no boot (IO45). Por isso ficam reservados.
- **STEP = IO3**, **DIR = IO21**. IO3 é ADC de bateria na SIMCAM; como não
  há bateria no projeto, foi liberado para digital/PWM.
- Nível lógico dos sinais da placa é **3,3 V**. Não aplicar 5 V nos pinos.
- Driver é **DM556D (Azaneli)**, alimentado com **24 VDC**.
- A **SIMCAM detecta a pessoa e comanda o pan**: a visão roda na própria
  ESP32-S3, sem câmera externa, sem controladora separada e sem PC no laço.
  O erro de posição detectado vira o alvo do eixo único.
- **A câmera gira junto com o duto/ventilador** (decisão do usuário em
  20/11/2026): placa, câmera e saída de ar formam um único conjunto móvel
  acionado pelo motor. É uma **malha fechada** — quando o pan gira, o quadro
  gira junto e a detecção precisa compensar esse movimento. O objetivo é manter
  a pessoa centralizada no quadro **e** o fluxo de ar apontado a ela.
- Um único motor de passo aciona esse conjunto, via DM556D @ 24 VDC.
- Não existem fim de curso, módulo relé, laser nem luz neste projeto.

## NAO CONFIRMADO (não montar sem verificar)

- Modelo/corrente/tamanho do motor e torque necessário para o duto.
- Tipo de entrada do DM556D Azaneli (comum-anodo vs. comum-catodo), corrente
  de entrada dos optos e necessidade de conversor 3,3 → 5 V. Mesmo modelo
  nominal pode ter revisão diferente do DM556D usado no laserseg.
- Tensão/potência da fonte 24 V, fusível, bitolas e dissipação.
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
