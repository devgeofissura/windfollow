# Estado do ventiseg

Atualizado em 20/11/2026.

## Fase atual

**Bancada de lógica — sem energizar motor.** O firmware é serial-only por
construção (`VENTISEG_ENABLE_GPIO=0`). Nenhuma gravação com GPIO ativo foi
feita.

## Confirmado nesta entrega

- Perfil `ventiseg` registrado no `tools/flash.ps1` e na tabela de perfis.
- Pinagem STEP=IO3, DIR=IO21, IO45/46 reservados por strapping.
- Driver DM556D Azaneli @ 24 VDC e pan do fluxo de ar.
- **A SIMCAM detecta a pessoa e comanda o pan** — visão e controle na mesma
  placa, sem controladora externa (reafirmado pelo usuário em 20/11/2026).
- **A câmera gira junto com o duto**: malha fechada; a visão compensa o
  próprio movimento do pan.
- Protocolo v0.1 especificado; `T` é o resultado da detecção interna.
  Porta UDP 4211 apenas reservada para depuração futura; serial-only agora.
- Guia executivo de ligação em `docs/LIGACOES.md` e esquema WireViz em nível
  de terminal (`PUL±/DIR±/ENA±`, fases `A±/B±`, interface 3,3 V → opto);
  variante comum-ânodo desenhada, validada pelo WireViz 0.4.1.
- Núcleo TDD entregue: `tools/protocol_ref.py` → `controller/parser.cpp` →
  52 vetores gerados → `autotest.h` → simulador, com **135 testes host
  verdes** (111 protocolo + 24 simulador). Watchdog renovado **somente** por
  `T` aceito no firmware e no simulador (divergência corrigida).
- Sketch compila na FQBN `ventiseg` (313.540 B flash / 22.960 B RAM), com
  `VENTISEG_ENABLE_GPIO=0` e nenhuma chamada de GPIO/Wi-Fi.

## Ainda não feito

- Autoteste **na placa** não executado: nenhuma gravação foi feita nesta
  entrega. Detectar a porta na hora antes de qualquer upload.

## Pendências antes de qualquer ensaio com motor

| ID | Pendência | Bloqueia |
|---|---|---|
| P1 | Conferir entradas PUL/DIR/ENA do DM556D Azaneli (comum e corrente) | Interface 3,3 V |
| P2 | Confirmar modelo/corrente do motor e ajuste de corrente do driver | Energização |
| P3 | Confirmar fonte 24 V, fusível, bitolas e aterramento | Energização |
| P4 | Definir curso útil, limite por software e batentes mecânicos | Movimento |
| P5 | Observar pulso no boot/reset e travamento com carga desacoplada | Segurança |

## Regra de atualização

Toda mudança de firmware atualiza este arquivo, `docs/PROTOCOLO.md`,
`docs/HARDWARE-DECISIONS.md` e `docs/DEVLOG.md` da raiz no mesmo commit.
