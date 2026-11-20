# Tarefas — Simulador Web V1

Plano: `tasks/plan-simulador-web.md`. Spec: `spec/SPEC-simulador-web.md`.

## Task 1 — Testes de contrato (RED)

- [x] Testes em `tests/test_simulator.py` para arquivos, constantes, mensagens,
      segredos e acessibilidade.
- **Aceite:** falham porque `simulator/` ainda nao existe.
- **Verificar:** `powershell -ExecutionPolicy Bypass -File firmware/laserseg/tests/run.ps1`.
- **Arquivos:** `tests/test_simulator.py` (1 arquivo — XS).

## Task 2 — Estrutura e tema

- [x] `index.html` com cabecalho, aviso de simulacao, abas e tres `section`s.
- [x] `css/style.css` com tokens do padrao Dracula/claro, responsividade e foco.
- **Aceite:** abre por duplo clique, alterna tema, navega pelas abas.
- **Verificar:** inspecao manual + `node --check` apos Task 3.
- **Arquivos:** `simulator/index.html`, `simulator/css/style.css` (2 — S).

## Task 3 — Logica do Painel

- [x] FSM `SAFE/TRACK/MANUAL`, comandos, `T`, `ACK`, watchdog, log e camera.
- **Aceite:** ligar/desligar rastreamento e luz gera mensagens; perda de link
      para eixos e luz; log mostra `ACK` com fault `LINK`.
- **Verificar:** testes host verdes + `node --check` + manual.
- **Arquivos:** `simulator/js/app.js` (1 — S).

## Task 4 — Wi-Fi e Protocolo

- [x] Fluxo AP → varredura → salvar → STA/fallback com log `PROV ...`.
- [x] Aba Protocolo somente leitura com constantes e formatos.
- **Aceite:** senha da rede nao aparece em log nem em arquivo; AP usa
      `laserseg`/`123456`.
- **Verificar:** testes host + manual.
- **Arquivos:** `simulator/js/app.js`, `simulator/index.html`,
      `simulator/README.md` (3 — M).

## Checkpoint

- [x] Suite completa verde (92 + novos), `node --check` limpo, revisao feita.

## Task 5 — Fechamento

- [x] Spec marcada como implementada; README/ESTADO/history atualizados;
      commit unico com DEVLOG e push.
- **Aceite:** `git status` limpo e entrada nova no DEVLOG.
- **Arquivos:** spec + 3 docs (4 — M).
