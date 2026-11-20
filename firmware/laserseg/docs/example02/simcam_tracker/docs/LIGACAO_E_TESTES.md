# Ligações e ensaio

## Arquitetura

A T-SIMCAM faz captura/interface e envia datagramas UDP pela rede local. Uma ESP32-S3 separada controla os motores e a luz. Não conectar motores à S3_CAM_SIM: além de corrente insuficiente, os GPIOs da câmera estão ocupados por periféricos e restrições da revisão.

```text
S3_CAM_SIM (captura + página web) -- Wi-Fi / UDP 4210 --> ESP32-S3 controladora
                                                             ├─ dois motores + drivers (um ramo)
                                                             ├─ quatro fins-de-curso
                                                             └─ estágio MOSFET --> foco LED difuso
```

## Driver da foto: Azanelli DM556D

A etiqueta mostra um **DM556D Microstep Driver**. Os terminais são PUL+/PUL− (passo), DIR+/DIR− (direção), ENA+/ENA− (habilitação), A+/A− e B+/B− (duas fases do motor), e GND/+Vdc para a alimentação do driver. A etiqueta marca alimentação de **24–50 Vdc**. O motor informado é **23HS7628-TB6600**, bipolar, 2 fases e 4 fios. A ficha comercial consultada para 23HS7628 lista 2,8 A por fase, 1,8° e 76 mm de comprimento, mas a etiqueta do seu exemplar deve prevalecer. Para pan e tilt são necessários dois drivers DM556D, um por motor.

O sketch `controller_dm556d` gera STEP/DIR nos GPIOs 4/5 para pan e 6/7 para tilt. **Não conecte os GPIOs diretamente aos terminais PUL/DIR sem consultar o manual da unidade.** A foto mostra pares ±, mas não informa sozinha tensão de sinal, corrente de entrada nem modo comum-ânodo/comum-cátodo. Use interface de sinal dimensionada para entradas do driver e lógica de 3,3 V da ESP32. O pino ENA pode ficar sem uso somente se o manual indicar que essa é a condição normal; não o conecte por suposição. Nunca aplique os 24–50 V da alimentação do motor aos terminais de sinal ou à ESP32.

SW1–SW3 escolhem a corrente de saída. A lista consultada informa 2,8 A por fase, mas a base RMS/peak desse anúncio não foi explicitada; confirme na etiqueta/ficha do motor. Como posição inicial conservadora, a tabela do DM556D oferece 2,7 A RMS / 3,8 A de pico com SW1=OFF, SW2=OFF, SW3=ON. Essa indicação não substitui a confirmação do fabricante do motor. SW4 seleciona meia corrente (OFF) ou corrente plena (ON); a decisão afeta torque de retenção e aquecimento. Desenergize antes de mudar DIP.

SW5–SW8 escolhem pulsos por volta. A linha de 3200 pulsos/volta na etiqueta indica SW5=OFF, SW6=OFF, SW7=ON, SW8=ON. Para motor de 1,8° (200 passos inteiros/volta), 3200 pulsos/volta equivalem a 16 microsteps por passo inteiro. O sketch usa `STEPS_PER_REV=3200` em coerência com essa posição; confirme o ângulo do motor e a posição física dos DIP antes de mover. Ajuste o código se escolher outra linha.

Para o motor, identifique as bobinas pelo diagrama do fabricante ou por medição cuidadosa e conecte cada fase a A+/A− ou B+/B−. Não escolha os pares pelas cores dos fios sem documentação. Para dois eixos, são dois drivers DM556D e dois motores compatíveis; a foto mostra somente um driver. NEMA 23 típico usa flange de 56,4 mm, furação de 47,14 mm e piloto de 38,1 mm, mas confira o desenho da unidade antes da impressão.

## Alternativa A — bipolar de 4 fios + DRV8834

Use somente se o driver físico for DRV8834 e o motor bipolar pequeno atender aos limites de tensão/corrente desse módulo. O sketch usa STEP/DIR: GPIO 4/5 no pan e 6/7 no tilt, além de controle de SLEEP nos GPIOs 8/9. Os enrolamentos ligam a A1/A2 e B1/B2. Configure microstepping e corrente na placa. **Não use esta pinagem de energia/sleep para o DM556D:** os dois drivers usam pulsos, mas seus terminais, alimentação e interface elétrica são diferentes.

## Alternativa B — 28BYJ-48 + duas placas ULN2003

Use uma placa ULN2003 para cada motor de 5 fios. Entradas IN1–IN4 do pan: GPIO 4, 5, 6, 7; tilt: GPIO 8, 9, 10, 11. Conecte o motor conforme sequência indicada na placa. Alimente o ULN2003 na tensão nominal do motor (muitos 28BYJ-48 são de 5 V; confirme a etiqueta) e una GND à controladora. Não alimente motores pelos pinos 3V3/USB da ESP32. Este ramo não usa DM556D.

## Alimentação, limites e foco LED

Para motores e LED, use fonte externa adequada aos componentes. No caso do DM556D, a etiqueta do driver indica 24–50 Vdc, mas a tensão escolhida também precisa ser compatível com motor/fonte e com o manual. Una referências de GND da lógica e do sinal apenas segundo a topologia de interface definida para o DM556D; mantenha separação apropriada no lado de potência. Não una saídas de fases entre drivers.

GPIO 13/14 são limites mínimo/máximo do pan; GPIO 15/16 do tilt. O sketch pressupõe interruptores normalmente fechados (NF) entre entrada e GND: fechado em operação normal; aberto no limite/fio rompido fica HIGH com pull-up e bloqueia movimento naquele sentido. Instale os quatro fins de curso. O limite lógico de ±60° é estimativa relativa ao centro manual; não há homing automático.

GPIO 12 comanda a entrada de um módulo MOSFET/driver para o foco LED. Use fonte própria para o foco, estágio apropriado e resistor de pull-down no gate se necessário. Use difusor, evite ofuscamento e verifique aquecimento.

## Rede Wi-Fi

No AP inicial, a câmera cria `SIMCAM-SETUP` (senha `simcam123`) em `192.168.4.1`. A controladora tenta usar `192.168.4.2`; UDP usa a porta 4210. Para STA, configure câmera e controladora no mesmo roteador e informe o IP da controladora na página da câmera. Alterar credenciais antes de uso fora de bancada; web/UDP não têm autenticação.

## Sequência de ensaio

1. Desligue a alimentação. O modelo 23HS7628 foi informado e uma listagem comercial indica 2,8 A/fase, mas fotografe a etiqueta elétrica do motor para confirmar corrente, passo e diagrama. A foto confirma um DM556D; para dois eixos, confirme o segundo driver.
2. Com o ramo DM556D, confirme que existem dois drivers se pretende dois eixos. Leia a corrente do motor e só então configure SW1–SW3; ajuste SW5–SW8 e `STEPS_PER_REV` de modo compatível. Faça todas as alterações sem energia.
3. Valide a interface STEP/DIR em bancada, inicialmente sem ligar os motores. Confirme polaridade, níveis, corrente nos optoacopladores e pulso mínimo no manual exato. Não teste ligando diretamente um GPIO 3,3 V por tentativa.
4. Confirme fins de curso e parser UDP. Grave a câmera e uma única variante de controladora. Centralize os eixos manualmente antes da partida.
5. Com movimento mínimo e sem carga, confirme direção, velocidade e limites. Monitore aquecimento e corrente; pare diante de ruído, travamento ou aquecimento anormal.
6. Teste perda de Wi-Fi, alvo parado e abertura de cada fim de curso. O controle desabilita movimento automático após observações inválidas/timeout e apaga a luz após perda de comunicação.

## Medidas necessárias para finalizar CAD e configuração

Enviar foto da etiqueta de cada motor, dos conectores e dos dois drivers; medir corpo, eixo e furos. Também medir placa/câmera, massa e foco. O CAD atual é paramétrico conceitual, ainda não validado em impressora nem dimensionalmente para o motor real.
