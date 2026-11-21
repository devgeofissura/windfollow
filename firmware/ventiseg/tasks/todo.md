# TODO — ventiseg

## Fase 0 — fundação (em andamento)

- [x] Detectar placa (COM4 · ESP32-S3 · 16 MB · `dc:da:0c:28:25:30`).
- [x] Fixar pinagem STEP=IO3, DIR=IO21, IO45/46 reservados.
- [x] Fixar driver DM556D Azaneli @ 24 VDC.
- [x] Escrever decisões de hardware e protocolo v0.1.
- [x] Perfil `ventiseg` em `tools/flash.ps1` + tabela da raiz.
- [x] Referência executável `tools/protocol_ref.py` + testes host.
- [x] Parser C++ serial-only + autoteste (52 vetores; 149 testes host verdes).
- [x] Simulador local + teste de espelhamento (watchdog alinhado ao contrato).
- [x] Esquema WireViz enxuto + guia de execução `docs/LIGACOES.md`.
- [x] Commit/push via `tools/save.ps1`.
- [x] Autoteste na placa: 85/85 checagens, `gpio_gate=0`, serial-only.
- [x] Auditoria P1–P5 documentada (referência + propostas; fisicamente aberta).

## Fase 1 — habilitação de GPIO (bloqueada por P1–P5)

- [ ] `VENTISEG_ENABLE_GPIO=1` apenas após auditoria.
- [ ] Ensaio com carga desacoplada.
- [ ] Calibração de `kMaxTravel` e `kTrackGain`.
