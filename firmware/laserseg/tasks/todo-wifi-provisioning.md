# Tasks: Provisionamento Wi-Fi (V1)

Plano de origem: `tasks/plan-wifi-provisioning.md`.
Spec aprovada: `spec/SPEC-wifi-provisioning.md` (2026-11-19).

Plano separado de `tasks/plan.md` / `tasks/todo.md` (DM556D + NEMA 23),
que seguem com T1, T5, T6 e T7 em aberto e não foram alterados.

## Fase 1 — Fundação testável

- [ ] **W1** — Espelho host da máquina de estados
  - `tools/wifi_provisioning_ref.py` + `tests/test_wifi_provisioning.py`.
  - NVS vazio → AP; salvo + conectado → STA; salvo + não conectado → AP sem retry.
  - Constantes conferidas: `laserseg`, `123456`, `15000`, `192.168.4.2`.
  - Verificar: suíte host verde.

- [ ] **W2** — Máquina de estados em C++
  - `lib/wifi_provisioning/provisioning_fsm.h`, sem dependência de `Arduino.h`.
  - `static_assert` das constantes; senha do AP em um único `constexpr`.

- [ ] **Checkpoint Fundação** — testes verdes, header sem rádio.

## Fase 2 — Módulo e integração na controladora

- [ ] **W3** — Módulo WiFi/WebServer/DNSServer/Preferences
  - AP `laserseg`/`123456`, portal com varredura, NVS, `ESP.restart()`,
    fallback AP e sem retry.
  - Somente bibliotecas do core; senha da rede nunca em serial/log.

- [ ] **W4** — Ligar o módulo ao nó DM556D
  - Editar `controller_dm556d/controller_dm556d.ino` mantendo autoteste e
    `LASERSEG_ENABLE_GPIO=0`.
  - Detectar placa, gravar e conferir `AUTOTEST PASS 76/76` + logs `PROV ...`.

- [ ] **Checkpoint Núcleo** — `laserseg-reviewer`, portal em bancada, nenhum
  GPIO ativado.

## Fase 3 — Documentação e fechamento

- [ ] **W5** — Documentação
  - `PROTOCOLO_UDP.md`, `ESTADO.md`, `README.md`, `HARDWARE-DECISIONS.md`.

- [ ] **W6** — DEVLOG, revisão e commit seletivo
  - Nunca `git add -A`: preservar `docs/example01/`, `docs/example02/` e `.zip`.

- [ ] **Checkpoint Final** — critérios de aceite da spec marcados, push feito.

## Estado atual

- Spec aprovada; plano proposto, aguardando revisão humana.
- Nenhum código de provisionamento escrito ainda.
- `LASERSEG_ENABLE_GPIO = 0`; nenhum GPIO, motor, luz ou câmera ativado.
