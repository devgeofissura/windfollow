# SIMCAM Tracker — Arduino CLI + pan-tilt

Projeto de protótipo para a **LILYGO S3_CAM_SIM V1.3** como câmera/interface e uma segunda ESP32-S3 como controladora. O usuário confirmou motor **NEMA 23** e mostrou um driver **Azanelli DM556D**. O ramo preferencial é DM556D, desde que o motor seja bipolar e sua corrente nominal caiba no ajuste do driver. Permanecem como alternativas o DRV8834 para motores bipolares menores e o ULN2003 para 28BYJ-48. Use apenas o ramo que corresponde ao conjunto físico.

## O que está implementado

- `camera_node/camera_node.ino`: captura JPEG, página web local, configuração Wi-Fi AP/STA, botão de luz, controles manuais e envio UDP.
- `controller_dm556d/controller_dm556d.ino`: alternativa STEP/DIR para DM556D; sinais elétricos ainda requerem interface apropriada entre ESP32 3,3 V e entradas do driver.
- `controller_drv8834/controller_drv8834.ino`: alternativa STEP/DIR para dois motores bipolares pequenos com DRV8834.
- `controller_uln2003/controller_uln2003.ino`: alternativa para dois motores 28BYJ-48 unipolares com placas ULN2003.
- `cad/pan_tilt_parametrico.scad`: suporte paramétrico com seletor entre colar 28BYJ e placa de motor bipolar com furação padrão NEMA 23 como ponto de partida.
- `docs/LIGACAO_E_TESTES.md`: ligações, alimentação, configuração e sequência de testes.

A foto identifica o **DM556D**, e o usuário informou o motor **23HS7628-TB6600**, bipolar de 2 fases e 4 fios. Uma ficha comercial do 23HS7628 lista 2,8 A por fase e passo de 1,8°; trate esse valor como provisório até conferir a etiqueta do seu exemplar. No DM556D, a posição mais próxima sem exceder esse valor indicado é 2,7 A RMS (SW1=OFF, SW2=OFF, SW3=ON), mas a base RMS/peak do anúncio pode não ser a mesma da tabela do driver. Verifique a etiqueta/manual do motor antes de manter essa configuração. Para pan e tilt são necessários **dois drivers DM556D, um por motor**.

O algoritmo de visão é **detecção de movimento por diferença entre quadros**, não um detector semântico. Pode acusar mudanças de iluminação, fundo ou câmera; não sabe que o movimento é de uma pessoa ou animal. Portanto, o código serve para ensaio de captura, interface e controle, e **não é um rastreador confiável de pessoas/animais**.

## Requisitos e bibliotecas

- Arduino CLI 1.6.0 e pacote `esp32:esp32` 3.3.12 no sandbox deste projeto.
- O core ESP32 fornece `esp_camera`, WiFi, WebServer, Preferences, WiFiUDP e conversão JPEG/RGB.
- `AccelStepper` 1.64.0 é usado nas controladoras.

Instalação, se necessário:

```bash
arduino-cli core update-index
arduino-cli core install esp32:esp32@3.3.12
arduino-cli lib install AccelStepper
```

## Compilar

Perfil recomendado para a câmera: ESP32-S3 Dev Module, 16 MB, PSRAM OPI, partição 16M/3MB APP, USB CDC ligado e 240 MHz.

```bash
FQBN='esp32:esp32:esp32s3:FlashSize=16M,PartitionScheme=app3M_fat9M_16MB,PSRAM=opi,USBMode=hwcdc,CDCOnBoot=cdc,CPUFreq=240'
arduino-cli compile --fqbn "$FQBN" camera_node
arduino-cli compile --fqbn "$FQBN" controller_dm556d
arduino-cli compile --fqbn "$FQBN" controller_drv8834
arduino-cli compile --fqbn "$FQBN" controller_uln2003
```

Ou execute `./compile_all.sh`. Na IDE Arduino, abra cada `.ino` dentro de sua própria pasta. A gravação/porta depende do cabo USB e do modo de boot da placa.

## Qual controladora escolher

**Para o 23HS7628 e driver da foto**, use `controller_dm556d`. Como ponto inicial provisório, a tabela do driver oferece 2,7 A RMS com SW1=OFF, SW2=OFF, SW3=ON, próximo mas ligeiramente abaixo dos 2,8 A/fase informados pela ficha comercial consultada. Confirme a corrente e a base de medição na etiqueta/manual do seu motor antes de energizar. A tabela do DM556D mostra 3200 pulsos/volta em SW5=OFF, SW6=OFF, SW7=ON, SW8=ON; para motor de 1,8°, isso corresponde a 16 microsteps por passo inteiro. O sketch usa 3200 como exemplo. SW4 seleciona meia corrente (OFF) ou corrente plena (ON); avalie retenção e aquecimento com a carga real.

A entrada PUL/DIR do driver não deve ser ligada diretamente à ESP32 sem confirmar os requisitos de tensão/corrente e a configuração comum-ânodo/comum-cátodo no manual. Use uma interface elétrica apropriada para lógica 3,3 V e para as entradas opto/diferenciais do driver. Não conecte 24–50 V aos GPIOs ou terminais de sinal.

Use `controller_drv8834` apenas com driver DRV8834 e motor bipolar compatível. Use `controller_uln2003` apenas com 28BYJ-48 de 5 fios e dois módulos ULN2003. A foto não confirma qual motor está em mãos.

## Rede e CAD

O AP inicial da câmera é `SIMCAM-SETUP`, senha `simcam123`; abra `http://192.168.4.1`. No AP, o firmware da controladora tenta usar `192.168.4.2` e recebe UDP na porta 4210. Em STA, configure câmera e controladora no mesmo roteador e informe o IP da controladora na página da câmera.

Edite `motor_type` em `cad/pan_tilt_parametrico.scad` como `"28byj"` ou `"bipolar"`. O padrão bipolar foi atualizado para flange NEMA 23 típica: face 56,4 mm, furação 47,14 mm e piloto 38,1 mm, baseado em desenho dimensional Nanotec. Confirme as cotas do seu motor antes de imprimir, pois fabricante/tolerâncias podem variar. O cubo impresso não é rolamento; em carga maior, apoie os eixos com mancais/rolamentos independentes.

## Estado de validação

Os quatro sketches compilaram com o ESP32 core 3.3.12. Isso não valida a interface elétrica com o DM556D, a captura na placa física, corrente do motor, movimento nem CAD impresso. O sufixo “TB6600” em algumas listagens parece indicar kit/opção de driver; o driver efetivamente fotografado é DM556D. O pinout da câmera segue o exemplo LilyGO `CAMERA_MODEL_TTGO_T_CAM_SIM`; confira esquema/revisão V1.3 se a câmera não iniciar.
