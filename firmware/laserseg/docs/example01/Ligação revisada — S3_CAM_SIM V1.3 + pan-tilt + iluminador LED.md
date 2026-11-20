# Ligação revisada — S3_CAM_SIM V1.3 + pan-tilt + iluminador LED

## Correção importante a partir da foto

A placa da foto está marcada **S3_CAM_SIM V1.3**. Os contatos laterais visíveis incluem IO31, IO46, IO45, IO21, GND e 3V3. Não use os quatro contatos IO para STEP/DIR: a própria LilyGO informa no issue do repositório que **GPIO31 está ligado à flash e indisponível; GPIO45 e GPIO46 são usados pelo UART do modem; GPIO21 está relacionado ao LED do mPCIe**. Ou seja, a sua placa não oferece GPIO extra confiável para ligar diretamente os dois drivers, o iluminador e os fins de curso. [1]

A solução robusta é manter a T-SIMCam como câmera/rastreador e acrescentar uma placa controladora com GPIOs suficientes — por exemplo, uma **ESP32-S3-DevKitC-1**. A T-SIMCam envia comandos de posição por Wi‑Fi à controladora; a placa adicional gera STEP/DIR, lê fins de curso e aciona o LED. Assim, não precisamos tocar nos sinais do modem nem nos pinos da câmera.

## Diagrama de blocos

```text
    USB-C                                      USB-C / 5 V regulados
      │                                                │
┌─────▼─────────────┐      Wi-Fi local          ┌─────▼──────────────┐
│ LILYGO S3_CAM_SIM │  ───────────────────────► │ ESP32-S3-DevKitC-1 │
│ V1.3 + câmera     │   posição/alvo pan-tilt   │ controlador        │
└───────────────────┘                           └──┬───┬───┬───┬────┘
                                                    │   │   │   │
                                                   STEP DIR STEP DIR
                                                    │   │   │   │
                                              ┌─────▼───┐ ┌─▼───────┐
                                              │ DRV8834 │ │ DRV8834 │
                                              │ PAN     │ │ TILT    │
                                              └────┬────┘ └────┬────┘
                                                   │           │
                                               Motor PAN   Motor TILT

Fonte motores (+) ───────── VMOT dos dois drivers
Fonte motores (−) ───────── GND dos dois drivers ─── GND da controladora

GPIO_LED da controladora ─ resistor ─ gate MOSFET N
Fonte LED (+) ─ módulo LED com driver/limitador ─ MOSFET ─ GND da fonte LED
GND da fonte LED ────────────────────────────────── GND da controladora
```

Como câmera e controladora se comunicam por Wi‑Fi, não é necessário ligar um GPIO da T-SIMCam aos drivers. Não interligue os sinais dos drivers com os contatos IO31/45/46/21 da placa da foto.

## Pinagem de exemplo para a placa controladora

A tabela abaixo é para **ESP32-S3-DevKitC-1**, não para a S3_CAM_SIM. Confirme a serigrafia/revisão da DevKitC-1 que comprar. Os pinos abaixo são uma proposta inicial e devem ser verificados contra as funções já habilitadas no firmware e o modelo exato da placa. A documentação oficial informa que a DevKitC-1 expõe os GPIOs utilizáveis em seus headers, exceto os reservados à flash/PSRAM. [2]

| Sinal | GPIO sugerido na ESP32-S3-DevKitC-1 |
|---|---:|
| STEP PAN | GPIO4 |
| DIR PAN | GPIO5 |
| STEP TILT | GPIO6 |
| DIR TILT | GPIO7 |
| ENABLE/SLEEP dos dois drivers (compartilhado) | GPIO15 |
| Acionamento do iluminador (`LED_EN`) | GPIO16 |
| Fim de curso PAN — referência | GPIO17 |
| Fim de curso TILT — referência | GPIO18 |
| Parada/intertravamento (opcional, além de corte físico) | GPIO21 |

Não conecte motor ou LED diretamente a esses GPIOs: eles fornecem apenas sinal lógico de 3,3 V.

## Ligações dos drivers (dois motores bipolares)

Para cada carrier DRV8834:

- `VM/VMOT` → positivo da fonte externa dos motores. O carrier Pololu especifica 2,5–10,8 V; confirme a faixa do módulo adquirido. [3]
- `GND` → negativo da fonte dos motores e GND da ESP32-S3-DevKitC-1. O terra comum é necessário para STEP/DIR.
- `STEP` → GPIO indicado para STEP do eixo correspondente.
- `DIR` → GPIO indicado para DIR do eixo correspondente.
- `SLEEP/nSLEEP` → GPIO_ENABLE compartilhado ou nível alto através de circuito de habilitação; configure para ficar desabilitado durante boot/reset. No carrier Pololu, `SLEEP` inicia baixo e precisa ficar alto para operar. [3]
- `ENBL` → conforme documentação do carrier; no Pololu pode ficar desconectado, mas clone pode variar. Confira o esquema do seu módulo.
- `A1/A2` e `B1/B2` → as duas bobinas do motor. Identifique os pares por datasheet ou ohmímetro antes de ligar.
- `M0/M1` → selecione o microstep conforme o carrier; comece no padrão documentado.
- `FAULT` → opcional para entrada de diagnóstico, depois de confirmar níveis lógicos.

Coloque capacitor eletrolítico de pelo menos 47 µF entre VMOT e GND perto do driver/fonte, com polaridade e tensão corretas. Ajuste o limite de corrente do driver antes de conectar carga. Desligue a fonte antes de conectar/desconectar o motor. A Pololu alerta que picos de tensão podem danificar o carrier. [3]

## Iluminador LED

Use um foco LED difuso de baixa tensão com **driver ou limitação de corrente próprios**. Não ligue um LED de potência nu diretamente à fonte. Para um módulo DC que permita chaveamento de baixa lateral, conecte: positivo da fonte LED ao positivo do módulo; negativo do módulo ao DRAIN de MOSFET N logic-level; SOURCE ao negativo da fonte; GPIO16 ao GATE através de resistor de 100–220 Ω; resistor de 10 kΩ entre GATE e GND para manter apagado durante reset. Una o GND da fonte LED ao GND da controladora.

Se o foco tiver entrada `DIM/EN`, prefira comandar essa entrada conforme o datasheet do próprio foco. Escolha fonte, MOSFET e fusível pela tensão/corrente especificadas pelo módulo LED; não dimensione sem essa etiqueta/datasheet. Use difusor e posicione a luz para iluminar a cena sem ofuscar o sujeito.

## Fins de curso e alimentação

Instale fins de curso nos extremos do curso mecânico; dois por eixo permitem limitar ambos os sentidos. Para referência de homing, pode-se começar com um por eixo. Ligue contatos normalmente fechados entre as entradas sugeridas e GND, habilitando pull-up interno ou instalando pull-up apropriado. O corte de emergência do LED e a desabilitação dos drivers devem ter estado padrão seguro durante reset.

- Alimente a S3_CAM_SIM V1.3 pela USB-C ou método indicado pela documentação da revisão.
- Alimente a DevKitC-1 por USB-C ou fonte 5 V regulada no pino permitido pela documentação da placa; não injete 5 V no pino 3V3.
- Alimente os motores por fonte externa separada, dimensionada pela corrente de fase e pelo torque necessário. Não use o 3V3 da câmera/controladora para motores.
- Mantenha o retorno de corrente dos motores entre fonte e drivers. Una GND da controladora ao GND dos drivers para referência lógica.
- Ponha fusível próximo à fonte dos motores e, se necessário, no ramo do iluminador; dimensione após saber as correntes reais.

## Referências

[1]: https://github.com/Xinyuan-LilyGO/LilyGo-Camera-Series/issues/33 "LilyGO issue: Usable IO pins in T-SIMCAM"
[2]: https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32s3/esp32-s3-devkitc-1/user_guide_v1.1.html "Espressif ESP32-S3-DevKitC-1 User Guide"
[3]: https://www.pololu.com/product/2134 "Pololu DRV8834 Low-Voltage Stepper Motor Driver Carrier"
