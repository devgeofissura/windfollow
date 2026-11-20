# SPEC-v0.1 — ventiseg

## Objetivo

Entregar a fundação de firmware para um eixo único de pan de fluxo de ar,
com visão na T-SIMCAM e acionamento por DM556D @ 24 VDC, mantendo o motor
**desenergizado** até a auditoria elétrica.

A SIMCAM detecta a pessoa com a própria câmera e **comanda o pan** a partir
dessa detecção. Não existe controladora externa, câmera externa ou PC no laço.
**A câmera gira junto com o duto**, então a malha é fechada: o erro de posição
detectado (`x` em `[-1000,1000]`) é convertido em alvo do eixo único e o
movimento altera o quadro seguinte, exigindo compensação. A telemetria `ACK`
volta na serial. O modelo de detecção (FPS, resolução, limiar), a escala
erro→graus e a montagem da câmera no conjunto girante são `NAO CONFIRMADO` e
entram na Fase 1, não nesta fundação.

## Escopo desta versão

- [x] Documentar hardware confirmado e pendências.
- [x] Definir contrato de protocolo com watchdog e boot seguro.
- [x] Implementar referência executável em Python.
- [x] Espelhar o parser em C++ no sketch `controller/controller.ino`.
- [x] Gerar 52 vetores de autoteste. Falta validar na placa (somente serial).
- [x] Entregar simulador web local com o mesmo contrato.
- [x] Registrar perfil `ventiseg` no `tools/flash.ps1`.
- [x] Gerar esquema WireViz enxuto + guia `docs/LIGACOES.md` (terminais,
      interface 3,3 V → opto, fases do motor, 24 V e terra comum).

## Fora de escopo

Laser, iluminador, relé, fim de curso, tilt, Wi-Fi ativo, MQTT, display.

## Critérios de aceite

1. `tests/run.ps1` verde na host.
2. Parser rejeita comando inválido sem mudar estado.
3. Watchdog força `state=SAFE`, eixo parado e `fault=Lnk`.
4. Nenhuma escrita em GPIO se `VENTISEG_ENABLE_GPIO=0`.
5. Simulador e firmware compartilham as mesmas constantes, com teste que
   detecta divergência.
