---
name: espressif-lilygo
description: Use para qualquer trabalho com ESP32/ESP32-S3 da Espressif e placas LILYGO T-SimCam/S3_CAM_SIM, especialmente no laserseg. Cobre toolchain Espressif, versões de core, FQBN/partição, pinagem oficial V1.3/V1.4, diferença de microfone e regras de detecção/gravação segura. Carregue antes de compilar, gravar ou mexer em GPIO.
category: embedded
version: 0.1.0
author: laserseg
tags: [esp32, esp32s3, espressif, lilygo, t-simcam, fqbn, gpio, flash]
compatible:
  - opencode
provenance:
  sources:
    - name: Xinyuan-LilyGO/T-SimCam
      url: https://github.com/Xinyuan-LilyGO/T-SimCam
      license: MIT
      verified: 2026-11-19
    - name: grantwilk/espressif-skills
      url: https://github.com/grantwilk/espressif-skills
      license: MIT
      verified: 2026-11-19
    - name: Espressif ESP-IDF / Arduino-ESP32 docs
      url: https://docs.espressif.com/projects/arduino-esp32/en/latest/
      license: documentation
      verified: 2026-11-19
---

# Espressif + LILYGO (laserseg)

## Objetivo

Evitar dois erros caros neste projeto: (1) trocar a placa/revisão e assumir pinos
iguais; (2) usar ferramenta Espressif sem antes identificar chip, porta e partição.
Esta skill é a camada de projeto sobre as skills globais `esp-*`.

## Quando usar

- Compilar ou gravar qualquer ESP32/ESP32-S3 do subprojeto.
- Escolher FQBN, core Arduino-ESP32, partição, PSRAM ou velocidade USB.
- Mexer em pinagem da T-SimCam V1.3/V1.4 ou em periféricos onboard.
- Interpretar falha de conexão, `esptool`, `arduino-cli` ou porta serial.

Quando a dúvida for só de uma ferramenta, carregue a skill atômica correspondente:
`esp-toolchain-overview` (índice), `esp-detect-device`, `esp-chip-info`,
`esp-idf-build`, `esp-flash-project`, `esp-troubleshoot`.

## Fatos confirmados (fonte: repo oficial LILYGO, MIT)

A. Não confundir revisões: **V1.3 = microfone I2S** (BCLK 41, WS 42);
**V1.4 = microfone PDM** (CLK 42). Câmera, SD e modem são iguais nas duas.

B. Arduino-ESP32 **3.3.11**; serial **115200**. A placa não tem display: todo
diagnóstico sai pela serial. `BOARD_POWER_ON_PIN = 1` precisa ser posto em HIGH
antes de usar qualquer periférico onboard.

C. Configuração Arduino IDE confirmada: ESP32S3 Dev Module, USB CDC On Boot
habilitado, CPU 240 MHz, Flash QIO 80 MHz / 16 MB, PSRAM OPI,_partition
`16M Flash (3MB APP/9.9MB FATFS)`, USB Mode Hardware CDC and JTAG, Upload
UART0/Hardware CDC a 921600.

D. PlatformIO confirmado: plataforma `pioarduino/platform-espressif32` release
`55.03.311`, `board = esp32s3box`, `board_build.partitions =
app3M_fat9M_16MB.csv`, `framework = arduino`, flags `-DBOARD_HAS_PSRAM
-DARDUINO_USB_CDC_ON_BOOT=1 -DARDUINO_USB_MODE=1`, `-DT_SIMCAM_BOARD_VERSION=13`
(V1.3) ou `14` (V1.4).

E. O sensor da câmera **não foi confirmado** pelo repo oficial consultado. Não
escrever OV2640/OV5640 como fato sem ensaio leitura de `sensor_id`.

### Pinagem T-SimCam (GPIO = número Arduino)

| Periférico | Pinos |
|---|---|
| Câmera | XCLK 14; SCCB SDA 4 / SCL 5; D2..D9 = 11,9,8,10,12,17,16,15; VSYNC 6; HREF 7; PCLK 13; PWDN/RESET = -1 |
| Microfone | DATA 2; I2S BCLK 41 + WS 42 (V1.3); PDM CLK 42 (V1.4) |
| microSD | MISO 40, MOSI 38, SCLK 39, CS 47 |
| Modem | RX 46, TX 45, power key 48, status 21 |
| Outros | power enable 1, BOOT 0 (ativo baixo), bateria ADC 3 (divisor 2:1), IR-cut 18, UART aux RX 44 / TX 43 |

GPIO43/44 também podem ser I2C, mas **não têm pull-up** e por padrão são UART
outputs. Não assumir barramento I2C livre sem resistor externo.

## Fluxo obrigatório antes de gravar

1. `esp-toolchain-overview` → convenções e índice.
2. `esp-detect-device` → listar portas; nunca reutilizar COM de sessão anterior.
3. `esp-chip-info` → confirmar chip, revisão, MAC e flash antes de `flash`.
4. Verificar partição compatível com o sketch (câmera + PSRAM 16 MB). Conferir
   o `PartitionScheme=app3M_fat9M_16MB` dos protótipos; o perfil `windfollow`
   da raiz usa `huge_app` — **não misturar**.
5. Compilar em `$env:TEMP` (nunca criar pasta de build no repositório).
6. Só então gravar, com foco desligado e eixos centralizados manualmente.

Se `arduino-cli` não estiver no PATH, usar `esp-idf-activate-env` / localizar a
ferramenta antes de qualquer comando. Não inventar `-p COM3` fixo.

## Regras de segurança

- Antes de `erase-flash`, `espefuse burn-*`, secure boot ou sobrescrever device
  com dados: **pedir confirmação explícita**. Leitura é sempre segura.
- Fazer backup do que for substituído (arquivo ou dump) e mostrar o diff.
- `BOARD_POWER_ON_PIN` HIGH antes dos periféricos; desligar LED/foco no boot,
  reset, perda de link e watchdog.
- A controladora ESP32-S3 separada continua responsável por motores e luz; não
  usar GPIO da T-SimCam para isso sem nova decisão registrada.

## Critério de saída

Você consegue responder: qual revisão da placa, qual core/partição, quais GPIOs
são reservados, qual a porta detectada agora e qual skill atômica usar em seguida.
Se sensor, chip ou revisão não foram confirmados, dizer "não confirmado".

## Referências

- [T-SimCam oficial](https://github.com/Xinyuan-LilyGO/T-SimCam) — README.MD, `platformio.ini`, `examples/*/config.h`
- [Arduino-ESP32](https://docs.espressif.com/projects/arduino-esp32/en/latest/)
- [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/stable/)
- Skills globais: `esp-toolchain-overview`, `esp-detect-device`, `esp-chip-info`, `esp-troubleshoot`
- Locais: `esp32-build-workflow`, `hardware-safety`, `laserseg-context`
