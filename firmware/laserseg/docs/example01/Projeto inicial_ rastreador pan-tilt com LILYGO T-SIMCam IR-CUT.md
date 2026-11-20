# Projeto inicial: rastreador pan-tilt com LILYGO T-SIMCam IR-CUT

## Conclusão de compatibilidade

**Sim: o ESP32-S3 atende ao processamento e ao controle de dois motores de passo.** A T-SIMCam é uma placa com ESP32-S3 e câmera DVP, e há suporte para ESP32-S3 e para OV2640 no driver `esp32-camera` da Espressif. A placa também tem PSRAM, importante para buffers de imagem. Para a primeira versão, use JPEG em resolução VGA ou SVGA e processamento de visão simples; o S3 não é um acelerador de visão computacional geral, então não se deve esperar inferência pesada em alta taxa de quadros sem reduzir resolução ou transferir o processamento para outro computador. [1] [2]

Há bibliotecas para os dois blocos: `esp32-camera` para capturar quadros e `FastAccelStepper` para gerar sinais STEP/DIR com aceleração no ESP32-S3. `AccelStepper` também é uma alternativa conhecida em Arduino. A câmera e os motores podem funcionar no mesmo firmware, mas os motores **não** devem ser alimentados pelos GPIOs nem pelo regulador da placa. [2] [3]

## Definição segura do protótipo

Este memorial trata o “laser seguidor” como **apontador de baixa potência para uma área de teste**, não como ferramenta para atingir/alvejar pessoas, animais, veículos, aeronaves ou propriedades. Para o primeiro protótipo, recomendo substituir o laser por um LED vermelho de alta intensidade. Se for indispensável usar laser, use um módulo visível certificado e rotulado como Classe 1 ou Classe 2, com potência nominal declarada de até 1 mW, montado para apontar somente para um anteparo próximo e fosco. A FDA alerta que feixes podem causar lesão ocular e que apontar para motoristas/operadores pode provocar acidentes. [4]

## Hardware recomendado

- 1 × LILYGO T-SIMCam IR-CUT com antena/cabo e USB-C para programação.
- 2 × motores de passo bipolares pequenos, com corrente nominal e torque especificados. Como ponto de partida mecânico, avalie NEMA 8 ou NEMA 11; dimensione após conhecer o peso da câmera, suporte e braço. Não compre apenas pelo tamanho.
- 2 × drivers STEP/DIR com entradas lógicas compatíveis com 3,3 V, por exemplo módulos baseados em DRV8834, dimensionados para a corrente do motor e montados com dissipação adequada. Ajustar o limite de corrente conforme datasheet do motor e do módulo.
- 1 × fonte externa para motores, escolhida conforme tensão/corrente dos motores e dos drivers. Não usar a saída 3,3 V da T-SIMCam para motores.
- 1 × conversor buck regulado se a fonte dos motores não puder alimentar diretamente os módulos/elementos auxiliares; fonte da placa separada por USB ou alimentação aprovada pela documentação da revisão.
- 1 × suporte pan-tilt com dois eixos, mancais, acopladores e base rígida. Colocar o eixo de tilt próximo ao centro de massa para reduzir torque e vibração.
- 2 × fins de curso mecânicos (um por eixo; dois por eixo é melhor para limitar os dois extremos) ou sensores equivalentes.
- 1 × botão físico de parada/armamento, normalmente aberto para habilitar, com estado padrão desabilitado.
- 1 × LED indicador de estado; opcional: expansor I²C de GPIO para fins de curso e entradas auxiliares se os GPIO acessíveis forem insuficientes.
- Se usar laser: módulo certificado de baixa potência; transistor MOSFET de canal N com acionamento adequado a 3,3 V; resistor de gate e resistor pull-down; chave de habilitação/intertravamento em série com a alimentação do módulo. O GPIO só comanda o MOSFET, nunca conduz diretamente a corrente do laser.
- Cabos, conectores, protoboard para bancada (não para montagem final com motores), placa perfurada/PCB, fusível na alimentação dos motores e capacitores de desacoplamento conforme especificação dos drivers.

## Ligações e arquitetura

1. A T-SIMCam captura quadros da câmera IR-CUT. A lógica de visão calcula o centro do objeto de teste e a diferença em pixels entre esse centro e o centro da imagem.
2. O controlador converte o erro horizontal/vertical em posições-alvo pan/tilt, com zona morta, limite de velocidade, aceleração suave e limites mecânicos. Comece com um alvo impresso e estático.
3. Quatro GPIOs livres e confirmados na **sua revisão** vão aos pinos STEP e DIR dos dois drivers. ENABLE pode ser compartilhado entre drivers se a lógica dos módulos e o estado seguro forem verificados. Fins de curso vão a GPIOs restantes ou expansor I²C.
4. A fonte externa alimenta os drivers e motores. Una o GND da fonte de motor ao GND lógico da T-SIMCam para referência de sinal, mantendo a corrente dos motores fora das trilhas de alimentação da placa.
5. O laser/LED só pode ser habilitado quando o botão de armamento estiver acionado, a câmera estiver funcional, os limites estiverem válidos e não houver falha. Na inicialização, reset, perda de câmera ou watchdog, o laser deve desligar e os drivers devem entrar em estado seguro.

**Não fixei os números dos GPIOs neste memorial.** A documentação da LilyGO apresenta tabelas diferentes para T-SIMCam e outras placas da família, e o mapa público de pinos do repositório possui uma tabela cujo cabeçalho identifica outro modelo. Além disso, “IR-CUT” pode indicar revisão ou conjunto de câmera distinto. Use o esquema da revisão exata e a serigrafia antes de conectar qualquer fio; não reutilize o pinout de T-CameraS3 ou T-Camera Plus. [1] [5]

## Firmware e biblioteca

Comece em Arduino IDE ou PlatformIO com os parâmetros de placa recomendados pela LilyGO para sua revisão, PSRAM habilitada e o exemplo oficial de câmera. Inicialize `esp32-camera` com os GPIOs da sua placa, JPEG, resolução VGA/SVGA e poucos frame buffers. Depois adicione `FastAccelStepper`, atribuindo um motor por par STEP/DIR. Faça primeiro teste individual de câmera, de cada motor sem carga, de sentido de rotação, de fins de curso e do corte de laser/LED; só depois integre o rastreamento.

O rastreamento mínimo pode ser por diferença de imagem, cor/contraste controlado ou marcador ArUco/QR em baixa resolução; para detecção genérica robusta, prefira enviar quadros a um computador ou servidor e receber coordenadas, em vez de presumir que o ESP32-S3 executará qualquer modelo de visão em tempo real. A interface pode ser Wi-Fi local ou 4G, caso o modem esteja instalado e configurado.

## O que preciso para fechar pinagem e firmware

Envie uma foto nítida dos dois lados da placa, a revisão/serigrafia, a etiqueta do módulo de câmera, o modelo/corrente dos motores e o peso aproximado do conjunto pan-tilt. Com isso dá para validar GPIOs realmente expostos, escolher fonte/driver com corrente correta e entregar o esquema elétrico e firmware sem adivinhar.

## Referências

[1]: https://wiki.lilygo.cc/products/t-sim-series/t-simcam/ "LILYGO T-SIMCam documentation"
[2]: https://github.com/espressif/esp32-camera "Espressif esp32-camera driver"
[3]: https://docs.arduino.cc/libraries/fastaccelstepper/ "Arduino FastAccelStepper library"
[4]: https://www.fda.gov/radiation-emitting-products/laser-products-and-instruments/frequently-asked-questions-about-lasers "FDA Frequently Asked Questions About Lasers"
[5]: https://github.com/Xinyuan-LilyGO/LilyGo-Camera-Series "LILYGO Camera Series repository"
