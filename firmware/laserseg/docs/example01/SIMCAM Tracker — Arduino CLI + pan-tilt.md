# SIMCAM Tracker — Arduino CLI + pan-tilt

Projeto de protótipo para a **LILYGO S3_CAM_SIM V1.3** como câmera/interface e uma segunda ESP32-S3 como controladora. Há duas versões alternativas de controladora: **motor bipolar de 4 fios + DRV8834** e **28BYJ-48 + ULN2003**. Não conecte os dois conjuntos de drivers ao mesmo motor.

## O que está implementado

- `camera_node/camera_node.ino`: captura JPEG, página web local, configuração Wi-Fi AP/STA, botão de luz, controles manuais e envio UDP.
- `controller_drv8834/controller_drv8834.ino`: alternativa para dois motores bipolares por STEP/DIR.
- `controller_uln2003/controller_uln2003.ino`: alternativa para dois 28BYJ-48 unipolares com placas ULN2003.
- `cad/pan_tilt_parametrico.scad`: modelo paramétrico de suporte, com seletor entre colar 28BYJ e placa bipolar tipo NEMA14.
- `docs/LIGACAO_E_TESTES.md`: ligações, alimentação, configuração e sequência de testes.

O primeiro algoritmo de visão é **detecção de movimento por diferença entre quadros**, não um detector semântico. Pode acusar mudanças de iluminação, fundo ou câmera; não sabe que o movimento é de uma pessoa ou animal. Portanto, o código serve para ensaio de captura, interface e controle, e **não é um rastreador confiável de pessoas/animais**.

## Requisitos instalados e bibliotecas

- Arduino CLI 1.6.0 e pacote `esp32:esp32` 3.3.12 no sandbox deste projeto.
- O core ESP32 fornece `esp_camera`, WiFi, WebServer, Preferences, WiFiUDP e conversão JPEG/RGB; não instalar essas bibliotecas separadamente.
- `AccelStepper` 1.64.0 é usado nas duas controladoras.

Instalação da biblioteca, se necessário:

```bash
arduino-cli core update-index
arduino-cli core install esp32:esp32@3.3.12
arduino-cli lib install AccelStepper
```

## Compilar

Para a câmera, use o perfil LILYGO T-SIMCam recomendado pela documentação da placa: ESP32-S3 Dev Module, 16 MB, PSRAM OPI, partição 16M/3MB APP, USB CDC ligado e 240 MHz. Exemplo de FQBN:

```bash
FQBN='esp32:esp32:esp32s3:FlashSize=16M,PartitionScheme=app3M_fat9M_16MB,PSRAM=opi,USBMode=hwcdc,CDCOnBoot=cdc,CPUFreq=240'
arduino-cli compile --fqbn "$FQBN" camera_node
arduino-cli compile --fqbn "$FQBN" controller_drv8834
arduino-cli compile --fqbn "$FQBN" controller_uln2003
```

Na IDE Arduino, abrir cada `.ino` dentro da própria pasta. A gravação/porta depende do cabo USB, driver e modo de boot da placa. Se a porta não entrar em modo de gravação, seguir o procedimento BOOT/RESET da documentação da LILYGO.

## Seleção da alternativa

**Use `controller_drv8834`** se cada motor tiver 4 fios de bobinas e for bipolar, com corrente compatível com o driver. As opções de microstepping M0/M1 da placa devem corresponder a `STEPS_PER_REV` no sketch; o exemplo é 200 passos/rev × 1/16. Não assuma esse valor para o motor real.

**Use `controller_uln2003`** se cada motor for 28BYJ-48 azul, normalmente com 5 fios, e houver uma placa ULN2003 para cada motor. O valor de 4096 meia-etapas/volta é aproximado e deve ser calibrado.

## Configuração de rede

O AP inicial da câmera é `SIMCAM-SETUP`, senha `simcam123`; abra `http://192.168.4.1`. No modo AP, os sketches de controladora tentam usar IP estático `192.168.4.2` e recebem comandos UDP na porta 4210. A câmera web permite mudar o IP da controladora.

Para modo cliente/STA, escolha o modo STA na página da câmera, salve SSID/senha e configure também `NET_MODE=1`, `WIFI_SSID` e `WIFI_PASSWORD` no sketch da controladora. Descubra o IP da controladora no Serial Monitor e informe esse IP na página da câmera. O AP e a configuração STA não são equivalentes: a rede STA precisa permitir tráfego UDP entre os dois dispositivos.

## CAD

Edite `motor_type` no topo de `cad/pan_tilt_parametrico.scad` como `"28byj"` ou `"bipolar"`. Exporte peças individuais trocando `part` por `"base"`, `"motor_mount"`, `"yoke"`, `"camera_plate"`, `"light_bracket"` ou `"shaft_hub"`. Todas as dimensões são hipóteses de partida. Meça a placa, motor, furos, câmera e foco antes de imprimir a versão final. O eixo/cubo impresso não é rolamento; para câmera pesada, adicionar apoio mecânico independente ao eixo de pan e ao eixo de tilt.

## Estado de validação

A compilação é testada pelo mantenedor deste diretório durante a preparação. Isso não valida a câmera física, o modelo de sensor instalado, as correntes dos motores, a precisão do movimento, a estabilidade do Wi-Fi nem o CAD em uma impressora específica. O pinout de câmera adotado no sketch é o do exemplo LilyGO `CAMERA_MODEL_TTGO_T_CAM_SIM`; confira revisão e esquema da unidade se a inicialização falhar.
