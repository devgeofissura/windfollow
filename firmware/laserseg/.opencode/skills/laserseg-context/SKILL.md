---
name: laserseg-context
description: Use no início de qualquer tarefa no subprojeto laserseg, ao perguntar sobre escopo, arquitetura, decisões, pendências ou estado. Use também antes de editar firmware, protocolo, documentação ou configuração OpenCode deste subprojeto.
---

# Contexto do laserseg

## Objetivo

Colocar na memória de trabalho o mínimo necessário para não contradizer decisões já
tomadas e não tratar protótipo como produto.

## Passos

1. Leia, nesta ordem:
   - `firmware/laserseg/README.md`
   - `firmware/laserseg/docs/ESTADO.md`
   - `firmware/laserseg/docs/PROTOCOLO_UDP.md` (somente se a tarefa envolver comunicação entre nós)
   - `firmware/laserseg/docs/foco-luz-seguidor-001` (somente se envolver pinagem/ligação)
   - `firmware/laserseg/docs/base-de-conhecimento-rastreamento-com-s3-cam-sim.md`
     (somente se envolver visão/sensor/incógnitas)
   - `AGENTS.md` da raiz e `firmware/laserseg/AGENTS.md`
2. Resuma internamente (e ao usuário, se ele pediu status) em no máximo 6 linhas:
   arquitetura, decisões fixadas, pendência mais relevante, restrição de segurança.
3. Classifique a tarefa como: documentação, configuração, firmware de visão, firmware de
   controle, protocolo, hardware ou revisão.
4. Se a resposta depender de sensor, revisão de placa, GPIO livre, driver ou torque,
   **não assuma**: diga que é incógnita e peça confirmação ou ensaio.

## Regras

- `docs/example01/` e `docs/example02/` contêm protótipos compiláveis, não firmware de produção validado.
- A T-SIMCAM faz visão; a controladora ESP32-S3 faz motores e luz. Não inverter essa
  separação sem nova decisão registrada.
- Iluminador = **laser** via módulo relé serial RoboCore (decisão de 2026-11-20,
  registrada em `docs/wireviz/00-LEIA-ME.md`): Clock/Data 5 V exige conversor
  3,3→5 V, bobinas 12 V em fonte separada, contato NA + emergência + watchdog.
  Laser bloqueado na bancada até fechar P1–P6 e a auditoria elétrica; antigo
  `LED_EN`/GPIO12 está SUPERADO.
- Nenhuma mudança de protocolo sem atualizar os dois nós, `PROTOCOLO_UDP.md` e `ESTADO.md`.

## Critério de saída

Você consegue responder: o que muda, o que não pode mudar, qual incógnita bloqueia e
qual skill carregar em seguida.
