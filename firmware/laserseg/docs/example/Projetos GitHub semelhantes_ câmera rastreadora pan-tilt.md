# Projetos GitHub semelhantes: câmera rastreadora pan-tilt

## Resultado da busca

Localizei cinco projetos públicos com código de rastreamento de rosto/objeto e movimento pan-tilt. **Nenhum é uma réplica pronta da S3_CAM_SIM V1.3 com dois motores de passo e foco LED difuso.** Os quatro primeiros controlam servos; o quinto usa gimbal BLDC em Raspberry Pi. Ainda assim, eles ajudam a estudar o algoritmo de centralização e a divisão entre visão e controle mecânico.

| Projeto | Câmera/processador | Movimento | Código e documentação | Proximidade com sua arquitetura |
|---|---|---|---|---|
| [Gill003/Pi_Servo_Tracker](https://github.com/Gill003/Pi_Servo_Tracker) | Raspberry Pi + Picamera2; detecção OpenCV DNN/SSD MobileNet | Arduino via I²C controla 2 servos | Python, sketch Arduino, arquivos do modelo; sem esquema/BOM detalhado | **Mais interessante como exemplo de arquitetura separada:** processador de visão envia erro/posição para um controlador independente. Precisa trocar servo por driver STEP/DIR e adaptar câmera/firmware. |
| [aju22/Object-Tracking-OpenCV_Arduino](https://github.com/aju22/Object-Tracking-OpenCV_Arduino) | PC + webcam + Python/OpenCV | Arduino UNO via serial controla 2 servos | Código Python e Arduino, GIFs/imagens; sem esquema de ligação completo | Bom exemplo de protocolo simples com coordenadas entre visão e atuador. Rastreamento por cor/contorno, dependente de iluminação e calibragem HSV. |
| [LanaDelSlay/Arduino-Face-Tracking](https://github.com/LanaDelSlay/Arduino-Face-Tracking) | PC + webcam + Haar Cascade | Arduino via serial com 2 servos | Código Python e Arduino, arquivo `Schematic.png`, instruções de início | Exemplo simples de rosto→coordenadas→movimento. Não usa ESP32, steppers nem iluminação; servo horizontal contínuo é controle sem feedback de posição. |
| [pageauc/face-track-demo](https://github.com/pageauc/face-track-demo) | Raspberry Pi + Pi Camera ou webcam USB; OpenCV | 2 eixos com servos via GPIO ou controlador Pi-Pan | Scripts de rastreamento/configuração e instaladores; documentação antiga, sem BOM/esquema completo | Útil para estudar diferença de quadros, contornos e perda do alvo. O software é antigo e focado em Raspberry Pi, não em ESP32-S3. |
| [SaraEye/SaraKIT-Face-Tracking-MediaPipe-Raspberry-Pi-64bit](https://github.com/SaraEye/SaraKIT-Face-Tracking-MediaPipe-Raspberry-Pi-64bit) | Raspberry Pi 4 CM4 + câmera; MediaPipe/OpenCV | Gimbal com motores BLDC e interface SaraKIT/SPI | Código C++, Makefile, modelos e instruções; sem esquema/BOM detalhado | Referência de detecção facial/landmarks e comando pan-tilt, mas hardware e software são específicos de Raspberry Pi/SaraKIT, não de drivers de passo. |

## O que observar em cada projeto

**Gill003/Pi_Servo_Tracker** separa a captura e o processamento da atuação: o Raspberry Pi detecta um objeto e envia dados por I²C para um Arduino, que move dois servos. O código atual filtra a classe “teddy bear” e usa Picamera2 e OpenCV DNN; não é código para copiar para o ESP32. O padrão de comunicação entre dois controladores é o que mais se aproxima da solução recomendada para sua placa. [1]

**aju22/Object-Tracking-OpenCV_Arduino** usa webcam e processamento Python no computador; a posição relativa do objeto segue por serial para o Arduino. A seleção do alvo depende de limites HSV configurados pelo usuário e de contornos, então muda com a cor do alvo e as condições de luz. Pode ajudar a desenhar o formato das mensagens de coordenadas, mas não traz motores de passo nem um BOM elétrico. [2]

**LanaDelSlay/Arduino-Face-Tracking** oferece uma referência simples de Haar Cascade no computador e comandos seriais para Arduino. Há uma imagem de esquema no repositório, mas não um projeto detalhado para motor de passo ou foco de luz. A lógica dos servos é básica e aberta; convém tratá-la como demonstração, não como firmware de produção. [3]

**pageauc/face-track-demo** reúne exemplos de diferença de imagem, contornos e detecção facial em Raspberry Pi. Inclui variantes de controle com servos pelo GPIO e com sistema Pi-Pan. Os instaladores e o ambiente descritos são antigos, e a ligação elétrica completa não está documentada. [4]

**SaraKIT Face Tracking** processa landmarks faciais com MediaPipe em Raspberry Pi e traduz o deslocamento facial em comandos para um gimbal BLDC. A ideia de converter erro em pixels em movimento dos eixos é aproveitável, mas a dependência de Linux, MediaPipe, SPI e SaraKIT impede uso direto no S3_CAM_SIM. [5]

## Recomendação para seu protótipo

Use esses repositórios como referências de algoritmo e de comunicação, não como esquemas elétricos prontos. Para a S3_CAM_SIM V1.3, a arquitetura mais coerente continua sendo: câmera/rastreamento na placa LilyGO; envio de coordenadas por Wi‑Fi; uma placa controladora separada acionando os dois drivers STEP/DIR, fins de curso e habilitação do foco LED. A limitação de GPIOs da S3_CAM_SIM torna importante manter o controle de movimento em outra placa; nenhum dos cinco projetos resolve exatamente essa pinagem ou fornece um firmware drop-in para a LilyGO.

## Referências

[1]: https://github.com/Gill003/Pi_Servo_Tracker "Gill003 Pi Servo Tracker"
[2]: https://github.com/aju22/Object-Tracking-OpenCV_Arduino "aju22 Object Tracking OpenCV Arduino"
[3]: https://github.com/LanaDelSlay/Arduino-Face-Tracking "LanaDelSlay Arduino Face Tracking"
[4]: https://github.com/pageauc/face-track-demo "pageauc Face Track Demo"
[5]: https://github.com/SaraEye/SaraKIT-Face-Tracking-MediaPipe-Raspberry-Pi-64bit "SaraEye SaraKIT Face Tracking MediaPipe Raspberry Pi"
