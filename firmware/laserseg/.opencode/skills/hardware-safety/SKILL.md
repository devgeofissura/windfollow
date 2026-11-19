---
name: hardware-safety
description: Use antes de qualquer mudança em GPIO, pinagem, alimentação, motores, drivers, fins de curso, watchdog ou saída de luz do laserseg. Use também ao revisar esquema, ligação ou ensaio de bancada, e ao avaliar risco de laser/LED.
---

# Segurança de hardware — laserseg

## Princípio

Falha de software, cabo solto ou pacote inválido deve terminar em **eixos parados e luz
apagada**. Segurança não depende da câmera, do Wi-Fi ou do operador estar olhando.

## Ordem de precedência (da mais forte para a mais fraca)

1. Parada física / chave de emergência (quando existir).
2. Fins de curso NF em `INPUT_PULLUP`: HIGH bloqueia o sentido; fio rompido também bloqueia.
3. Watchdog de link: > 1200 ms sem pacote válido → parar e Luz OFF.
4. Mensagem inválida, sequência repetida, `valid=0` ou `TRACK,0` → parar.
5. Zona morta (`DEAD_BAND = 55`) e limite lógico (`MAX_TRAVEL`).

## Checklist obrigatório antes de mexer em hardware

- [ ] GPIOs da câmera confirmados no esquema da revisão da placa; sensor identificado
      (OV2640/OV5640 ainda é incógnita). Não usar `CAMERA_MODEL_TTGO_T_CAM_SIM` como prova.
- [ ] Controladora ESP32-S3 separada continua acionando motores e luz — não a S3_CAM_SIM.
- [ ] LED desligado no boot/reset (`OUTPUT` + LOW antes de qualquer `loop`), com MOSFET/driver
      dimensionado para a corrente real; GPIO não alimenta o foco diretamente.
- [ ] Laser apenas se Class 1/2, ≤ 1 mW, anteparo fosfo próximo, feixe abaixo da altura dos
      olhos e nunca dirigido a pessoas, animais, veículos ou aeronaves. Na dúvida, LED difuso.
- [ ] Fins de curso NF com pull-up; sem `INPUT_PULLDOWN` e sem depender de contato mecânico
      perfeito. Testar fio desconectado antes de energizar motor.
- [ ] Alimentação separada/capaz para motores; GND comum entre lógica e drivers; capacitor
      de decoupling; sem alimentar motor pelo regulador da placa.
- [ ] Driver compatível com o motor (DRV8834 bipolar ou ULN2003/28BYJ-48); corrente,
      microstepping e tensão conferidos. Não trocar `TRACK_GAIN` sem novo ensaio.
- [ ] Curso inicial ±60° em torno do centro definido manualmente; sem homing automático.
- [ ] Ensaio em bancada sem carga mecânica, com fontes limitadas e foco desligado primeiro.
- [ ] Nenhuma porta COM fixa em código/doc; detecção feita na hora da gravação.

## Regras de mudança

- Qualquer alteração de pinagem exige: esquema revisado + `foco-luz-seguidor-001` +
  `ESTADO.md` + revisão do `hardware-auditor`.
- Não habilitar laser para "melhorar contraste" sem registrar classe, potência e anteparo.
- Não usar a UI web da câmera para comandar luz sem o mesmo watchdog do link UDP.
- Se o projeto estiver em bancada com pessoas por perto, LED OFF é a decisão padrão.

## Critério de saída

Você lista os riscos aplicáveis à mudança, a mitigação de cada um e confirma que o estado
seguro prevalece mesmo com Wi-Fi ausente. Se algum item crítico estiver em branco, a mudança
fica bloqueada até ensaio ou confirmação do usuário.
