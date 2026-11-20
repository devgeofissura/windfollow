# Ligação de hardware — T-SIMCam IR-CUT, pan-tilt e foco LED

## Premissas

O circuito abaixo é uma referência para **dois motores bipolares de passo** comandados por drivers STEP/DIR baseados em DRV8834 e um foco LED branco difuso de baixa tensão. A luz acompanha a câmera e ilumina a cena; não há laser.

Os nomes `GPIO_PAN_STEP`, `GPIO_PAN_DIR` etc. são **rótulos, não números de pinos**. A documentação pública não permite fixar com segurança os GPIOs acessíveis na revisão IR-CUT da sua placa. Confirme o esquema e a serigrafia antes de conectar. A documentação da LilyGO mostra ESP32-S3, câmera e periféricos integrados; o repositório da série possui tabelas de pinos para modelos/revisões diferentes. [1] [2]

## Diagrama funcional

```text
                         USB-C / alimentação aprovada
                                      │
                         ┌────────────▼────────────┐
                         │ LILYGO T-SIMCam IR-CUT  │
                         │ ESP32-S3 + câmera       │
                         └─────┬────┬────┬────┬────┘
                               │    │    │    │
             GPIO_PAN_STEP ────┘    │    │    └──── GPIO_TILT_DIR
             GPIO_PAN_DIR ──────────┘    └───────── GPIO_TILT_STEP
                    │                                  │
          ┌─────────▼────────┐               ┌─────────▼────────┐
          │ DRV8834 PAN      │               │ DRV8834 TILT     │
          │ STEP, DIR, SLEEP │               │ STEP, DIR, SLEEP │
          │ A1/A2, B1/B2     │               │ A1/A2, B1/B2     │
          └──────┬─────┬─────┘               └──────┬─────┬─────┘
                 │     │                            │     │
             Motor PAN                            Motor TILT

  Fonte motores (+) ─────────────── VMOT dos dois DRV8834
  Fonte motores (−) ───────┬─────── GND dos dois DRV8834
                            └─────── GND da T-SIMCam (terra comum)

  Fonte LED (+) ─────────────── LED+ [módulo LED com limitação/driver]
  LED− ───────────────────────── DRAIN de MOSFET canal N
  SOURCE do MOSFET ───────────── GND da fonte LED
  GPIO_LED ── resistor 100–220 Ω ─ GATE
  GATE ── resistor 10 kΩ ───────── GND
  GND fonte LED ────────────────── GND T-SIMCam (terra comum)
```

O diagrama é conceitual. No esquema final, distribua os retornos de alta corrente dos motores diretamente entre fonte e drivers; não passe corrente de motor pelas trilhas finas ou pelo conector GND da placa.

## Conexões pino a pino

### Cada DRV8834 — repetir para PAN e TILT

| Pino/parte | Conexão |
|---|---|
| `VM` / `VMOT` | Positivo da fonte externa dos motores, dentro da faixa do carrier DRV8834. A página do carrier Pololu especifica 2,5–10,8 V. [3] |
| `GND` (potência) | Negativo da fonte dos motores. |
| `GND` (lógica) | Mesmo terra comum da T-SIMCam. Confirme que o módulo tem GND comum; no carrier Pololu, os GND são comuns. |
| `STEP` | Um GPIO livre da T-SIMCam, por exemplo o rótulo `GPIO_PAN_STEP`. Cada pulso avança um microstep. [3] |
| `DIR` | Outro GPIO livre, por exemplo `GPIO_PAN_DIR`. Nível define o sentido; pode ser invertido por software. [3] |
| `SLEEP` / `nSLEEP` | Controle habilitado em nível alto, ou GPIO de habilitação com pull-down externo/estado de inicialização seguro. No carrier Pololu, SLEEP começa baixo e precisa ficar alto para operar. [3] |
| `ENBL` | No carrier Pololu, pode permanecer desconectado para habilitar a ponte por padrão; verifique o módulo comprado, pois clones podem variar. [3] |
| `A1/A2`, `B1/B2` (ou `AOUT1/AOUT2`, `BOUT1/BOUT2`) | As duas bobinas do motor. Identifique os pares com datasheet ou multímetro. Não conecte fios de bobinas em ordem aleatória. |
| `M0`, `M1` | Deixe no modo padrão inicialmente conforme o carrier, ou fixe conforme tabela do fabricante; não deixe entradas em estado indefinido se o módulo não tiver resistores internos. |
| `FAULT` | Opcional: entrada de diagnóstico do ESP32, somente se a tensão/lógica do carrier for compatível. |

Use um capacitor eletrolítico de **pelo menos 47 µF** entre VMOT e GND, perto de cada driver ou do conjunto de alimentação, respeitando polaridade e tensão nominal. A Pololu alerta que picos LC podem danificar o carrier; evite cabos longos entre fonte e driver. Desligue toda a alimentação antes de conectar ou remover motores. [3]

Ajuste o limite de corrente dos drivers **antes** de operar, segundo a corrente por fase do motor e a documentação do carrier. Não deduza a corrente necessária apenas pela tensão nominal impressa no motor. O limite contínuo depende da placa, dissipação e refrigeração do módulo. [3]

### Iluminador LED

Use um módulo LED branco difuso com tensão nominal identificada e **limitador de corrente/driver próprio**. Não conecte um LED de potência nu diretamente à fonte. Para módulo de 5 V ou 12 V com driver integrado, faça a comutação de baixa lateral com MOSFET canal N logic-level, especificado para corrente e tensão do módulo. O LED só liga quando `GPIO_LED` está ativo.

Se o iluminador exigir um driver de corrente constante com entrada `DIM/EN`, prefira comandar essa entrada conforme o datasheet do driver, em vez de interromper a saída de corrente constante com o MOSFET. Use um difusor e dimensione o suporte e a ventilação conforme potência térmica do iluminador.

### Fins de curso e parada

Instale pelo menos um fim de curso de referência por eixo para homing; para limitar os dois extremos, use dois por eixo. Ligue cada contato normalmente fechado entre entrada de fim de curso e GND, com pull-up habilitado ou resistor externo apropriado. Use GPIOs confirmados ou expansor I²C, se faltar entrada. O botão de parada deve retirar a habilitação dos drivers e apagar o LED por hardware ou por uma linha de habilitação segura; não dependa apenas da tarefa de câmera.

## Alimentação

- **Motores/DRV8834:** fonte externa com tensão compatível com motor e driver, corrente dimensionada para os dois eixos. A faixa nominal da placa Pololu DRV8834 é 2,5–10,8 V. Nunca use o pino 3V3 da T-SIMCam para motores. [3]
- **T-SIMCam:** USB-C ou método de alimentação autorizado para a revisão da placa; não injete tensão em pinos sem conferir o esquema.
- **Iluminador:** fonte compatível com a tensão e potência do módulo LED. Pode ser fonte separada; una o GND ao terra lógico para que o sinal do MOSFET tenha referência comum.
- Coloque fusível próximo à fonte de motores e outro no ramo de iluminação, dimensionados após conhecer as correntes. Não dimensione a fonte no escuro: preciso dos modelos/correntes dos motores e potência/tensão do foco.

## GPIOs: não conectar ainda por número

São necessários, no mínimo, quatro GPIOs para STEP/DIR dos dois drivers e um GPIO para dim/enable do iluminador, além de entradas para fins de curso e habilitação. Não use GPIOs já ocupados pela câmera, PSRAM, modem celular, USB ou periféricos da placa. A LilyGO fornece documentação e esquemas da família, mas a tabela correta deve corresponder à revisão exata da sua unidade. [1] [2]

**Para eu trocar os rótulos por números reais e fechar a ligação:** envie foto nítida da frente e do verso da placa, foto da etiqueta/serigrafia da revisão e link ou modelo dos motores e do iluminador. Até validar isso, mantenha os cabos dos drivers desconectados dos GPIOs.

## Referências

[1]: https://wiki.lilygo.cc/products/t-sim-series/t-simcam/ "LILYGO T-SIMCam documentation"
[2]: https://github.com/Xinyuan-LilyGO/LilyGo-Camera-Series "LILYGO Camera Series hardware and firmware repository"
[3]: https://www.pololu.com/product/2134 "Pololu DRV8834 Low-Voltage Stepper Motor Driver Carrier"
