# Tasks: DM556D + NEMA 23 (V1)

Plano de origem: `tasks/plan.md`. Spec aprovada:
`spec/SPEC-dm556d-nema23.md` (2026-11-19).

## Fase 1 — Fundação

- [ ] **T1** — decisões de hardware (7 pontos da spec)
  - **BLOQUEADO:** aguardando o usuário/auditor. Nenhum GPIO foi tocado.
- [x] **T2** — preservar e isolar o núcleo de protocolo
  - Vetores regenerados byte-idênticos (SHA256 `F70985...`).
  - `controller/` intacto; é a fonte única do núcleo C++.
  - Novo nó inclui `../controller/parser.cpp` e `../controller/autotest.h`.
- [ ] **Checkpoint Fundação** — revisão humana (spec/plan aprovados; T1 em aberto)

## Fase 2 — Camada de movimento (serial-only)

- [x] **T3** — constantes e tempo monotônico
  - `tools/motion_config.py`, `controller_dm556d/driver_config.h`,
    `tests/test_motion.py` (8 testes verdes).
  - Funções puras reutilizadas do núcleo; `static_assert` contra divergência.
  - `unwrapMillis()` cobre o wrap do `millis()` antes de `feed()/poll()`.
- [x] **T4** — sketch `controller_dm556d` serial-only
  - `LASERSEG_ENABLE_GPIO=0`; nenhum pino configurado.
  - Compile `exit=0`; placa detectada (COM4/ESP32-S3/16 MB/MAC `dc:da:0c:28:25:30`).
  - Gravação verificada; autoteste na placa `76/76 PASS`.
  - Perfil `laserseg_dm556d` adicionado a `tools/flash.ps1`.
- [x] **Checkpoint Núcleo** — `laserseg-reviewer` aprovou: sem bloqueantes
  - Pendência registrada para T5: tratar `AutoTestResult.failed != 0` como
    falha fechada antes de habilitar qualquer saída física.
  - Pendência registrada para T6: `nowMs()` já prepara o relógio monotônico;
    conectá-lo a `Session::feed()/poll()` quando o UDP entrar.
  - `unwrapMillis()` agora provado por `static_assert` em `driver_config.h`.

## Fase 3 — Integração física (gate de hardware)

- [ ] **T5** — STEP/DIR, fins de curso NF e LED sob `LASERSEG_ENABLE_GPIO=1`
  - **BLOQUEADO:** 7 respostas de hardware + `hardware-auditor`.
- [ ] **T6** — UDP 4210 na controladora
  - **BLOQUEADO:** T5 + definição de rede/credenciais fora do repo.

## Fase 4 — Fechamento

- [ ] **T7** — documentação e commit único
  - **BLOQUEADO:** T1–T6. Decidir destino das alterações do usuário em
    `docs/example01/`, `docs/example02/` e do `.zip` antes de qualquer
    `git add -A`.

## Estado atual do firmware gravado

- Sketch: `controller_dm556d` (serial-only).
- Boot: `tracking_=false`, `light_=false`, eixos parados, nenhum GPIO tocado.
- Comando serial `run` repete o autoteste; heartbeat a cada 5 s.
