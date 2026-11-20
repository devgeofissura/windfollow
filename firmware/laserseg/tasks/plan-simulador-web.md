# Implementation Plan: Simulador Web do laserseg — V1

Source spec: `firmware/laserseg/spec/SPEC-simulador-web.md` (aprovada em 2026-11-20).

> Plano separado de `tasks/plan.md`/`tasks/todo.md` porque esses arquivos contem
> o plano T1–T7 ainda aberto; nada aqui sobrescreve ou fecha esse trabalho.

## Overview

Pagina estatica local (`file://`) com tres abas que simula o contrato UDP v0.1,
a maquina de estados de provisionamento Wi-Fi e a experiencia de operacao do
laserseg — sem firmware, GPIO ou placa. Constantes espelhadas do nucleo C++
sao defendidas por testes host.

## Architecture Decisions

- **HTML/CSS/JS puro**, sem framework, servidor ou build: `index.html` abre por
  duplo clique e funciona offline.
- **FSM em JavaScript com funcoes puras** (`nextState`, `targetFromError`,
  `linkExpired`, `formatTelemetry`) espelhando `tools/protocol_ref.py` e
  `controller/parser.cpp`; a fonte da verdade continua sendo o nucleo validado.
- **Contrato por teste de texto**: `tests/test_simulator.py` compara as
  constantes do JS com `parser.h`/`driver_config.h` por regex — mesmo padrao ja
  usado por `test_motion.py`.
- **UI sem `innerHTML` dinamico**: log e status usam `textContent`; nenhuma
  credencial digitada sai da memoria da pagina.
- **Incrementos verticais**: cada tarefa deixa a pagina abrivel e testavel.

## Task List

### Phase 1 — Contrato e base visual

- [ ] Task 1: escrever `tests/test_simulator.py` (RED) com constantes,
      mensagens, segredos e acessibilidade.
- [ ] Task 2: criar `index.html` + `css/style.css` com cabecalho, abas
      Painel/Wi-Fi/Protocolo, tema claro/escuro e responsividade.

### Checkpoint: base

- [ ] `tests/test_simulator.py` falha apenas pelo codigo ainda ausente (RED
      confirmado antes da implementacao).
- [ ] Html abre sem servidor apos Task 2; `node --check` preparado para o JS.

### Phase 2 — Logica e fluxos

- [ ] Task 3: `js/app.js` — estado `SAFE/TRACK/MANUAL`, comandos
      `TRACK/LIGHT/MANUAL`, amostras `T`, telemetria `ACK`, watchdog 1200 ms,
      log e camera (webcam + alvo sintetico).
- [ ] Task 4: `js/app.js` + `index.html` — aba Wi-Fi (AP, varredura, salvar,
      STA/fallback) e aba Protocolo (constantes e formatos, somente leitura).

### Checkpoint: fluxos

- [ ] Suite host verde (92 testes existentes + novos).
- [ ] `node --check firmware/laserseg/simulator/js/app.js` sem erros.
- [ ] Revisao de codigo/simulador antes de fechar.

### Phase 3 — Fechamento

- [ ] Task 5: atualizar spec (status implementada), `README.md`,
      `docs/ESTADO.md`, `docs/history.md`; revisar; commitar com `save.ps1`.

## Riscos e mitigacao

| Risco | Mitigacao |
|---|---|
| JS divergir do contrato validado | Teste de espelho das constantes e mensagens + `node --check`. |
| Usuario confundir simulacao com hardware | Aviso fixo no cabecalho e no Painel: nenhum GPIO/motor/luz real. |
| Credencial de rede vazar para o repo | Senha so em memoria da pagina; campo `type=password` sem `value`. |
| Sobrescrever plano T1–T7 | Arquivos proprio `plan-simulador-web.md`/`todo-simulador-web.md`. |

## Verificacao final

```powershell
powershell -ExecutionPolicy Bypass -File firmware/laserseg/tests/run.ps1
node --check firmware/laserseg/simulator/js/app.js
git status --short --branch
```
