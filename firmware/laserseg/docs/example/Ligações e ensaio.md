# Ligações e ensaio

## Arquitetura

A T-SIMCAM faz captura/interface e envia datagramas UDP pela rede local. Uma ESP32-S3 separada controla os motores e a luz. Não conectar motores à S3_CAM_SIM: além de corrente insuficiente, os GPIOs dessa placa estão ocupados por câmera/modem e há restrições específicas de revisão.

```text
S3_CAM_SIM (captura + página web) -- Wi-Fi / UDP 4210 --> ESP32-S3 controladora
                                                             ├─ dois motores + drivers (uma alternativa)
                                                             ├─ quatro fins-de-curso
                                                             └─ MOSFET/driver --> foco LED difuso
```

## Alimentação e terra

Use alimentação separada e regulada para motores e LED, dimensionada após confirmar as especificações reais. Una o GND das fontes de potência, dos drivers, do MOSFET e da controladora em referência comum. Não alimentar os motores pelos pinos 3V3 ou USB da ESP32. Desacople a fonte junto aos drivers conforme as recomendações do fabricante. Evite alimentar LED de potência direto de GPIO.

### Alternativa A — bipolar de 4 fios + DRV8834

Para cada motor, conectar as duas bobinas às saídas A1/A2 e B1/B2 do DRV8834, conforme identificação por multímetro/datasheet. Conectar STEP e DIR do eixo pan aos GPIO 4 e 5; STEP e DIR do tilt aos GPIO 6 e 7. Ligar SLEEP/SLP aos GPIO 8 e 9. O sketch coloca esses sinais em HIGH. RESET deve ficar em nível alto, conforme o breakout do driver; verificar se a placa já une RESET e SLEEP. Ajustar M0/M1 (microstepping) e `STEPS_PER_REV` de forma coerente. VMOT recebe fonte para o motor dentro da faixa do driver e do motor; GND potência e GND lógica comuns. Ajuste o limite de corrente segundo o datasheet do motor e breakout antes de energizar.

Este ramo **não** serve para o 28BYJ-48 de 5 fios com placa ULN2003. Não ligue bobinas sem confirmar pares e corrente.

### Alternativa B — 28BYJ-48 + duas placas ULN2003

Use uma placa ULN2003 para cada motor. Conectar as entradas IN1, IN2, IN3, IN4 do pan aos GPIO 4, 5, 6, 7; as entradas do tilt aos GPIO 8, 9, 10, 11. Conectar as saídas do driver ao conector de 5 fios do motor, mantendo a sequência indicada na placa. Alimentar `VCC` do ULN2003 com a tensão nominal do motor (muitos 28BYJ-48 são 5 V; confirmar a etiqueta) e GND à fonte externa. A placa controladora compartilha GND com ambos ULN2003. Não unir VCC do driver à saída 3V3 da ESP32.

### Limites mecânicos e foco LED

GPIO 13/14 são os limites mínimo/máximo do pan; GPIO 15/16, do tilt. O sketch pressupõe interruptores **normalmente fechados (NF)** entre entrada e GND: fechado em operação normal; ao chegar ao limite, o circuito abre e a entrada com pull-up fica HIGH. Um fio rompido também bloqueia movimento naquele sentido, condição mais segura. Instale os quatro interruptores antes de movimentar o suporte. O firmware bloqueia a direção correspondente quando o interruptor é acionado. Os limites lógicos de ±60° são estimativas a partir da posição inicial; centralize a montagem manualmente antes de ligar. O firmware não faz homing automático.

GPIO 12 comanda entrada de um módulo MOSFET/driver do foco. Use resistor de pull-down no gate se o módulo não tiver. A alimentação do LED é própria, com corrente e dissipação conforme a ficha técnica. Use lente/difusor que evite ofuscamento e permita ventilação.

## Wi-Fi

No AP inicial, a câmera cria `SIMCAM-SETUP` (senha `simcam123`) no endereço `192.168.4.1`. As controladoras tentam entrar nesse AP e usar `192.168.4.2`; a interface envia UDP para essa posição, porta 4210. Para STA, configurar a câmera na página web e compilar a controladora com `NET_MODE=1`, SSID e senha iguais. Ver o IP da controladora no Serial Monitor e preenchê-lo na página da câmera. Alterar as credenciais iniciais antes de uso fora de bancada. A página web e o UDP de controle não têm autenticação; manter apenas em rede confiável.

## Sequência de testes recomendada

1. Com tudo desligado, identificar o motor pela etiqueta, número de fios e placa driver. Confirmar sensor de câmera e revisão exata da S3_CAM_SIM.
2. Compilar os três sketches; gravar a câmera e **apenas uma** das duas controladoras.
3. Sem carga mecânica, ligar primeiro a ESP32 controladora e conferir que os fins-de-curso fechados são lidos como LOW. Abrir um fim de curso por vez e confirmar que o movimento em direção ao limite é bloqueado.
4. Confirmar câmera em `http://192.168.4.1` no modo AP. Só depois de imagem estável, testar o botão de luz e um clique manual por vez.
5. Centralizar os eixos manualmente e testar pan/tilt com movimento pequeno. Monitorar aquecimento de motor/driver e tensão da fonte. Parar se houver ruído forte, vibração, perda de passos ou aquecimento anormal.
6. Habilitar rastreamento. O indicador da UI é apenas diferença de quadros; experimente sem carga, com cena estática e depois com movimento. Não usar como rastreador semântico nem deixar a montagem sem supervisão.
7. Testar perda de Wi-Fi, reinício, cabo de fim de curso rompido e alvo parado; sem observações recentes, o firmware deve deixar de seguir e parar.

## Pendências para versão mecânica final

Enviar fotos nítidas do motor, etiqueta, placa driver e conector; medir diâmetro/comprimento do corpo, eixo e distância dos furos. Também medir placa/câmera e posições dos furos, massa do conjunto e dimensões do foco. O modelo atual é um suporte ajustável conceitual, não um CAD validado para impressão final.
