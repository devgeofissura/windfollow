# Estado do ventiseg

Atualizado em 21/11/2026.

## Fase atual

**Bancada de lógica — sem energizar motor.** O firmware é serial-only por
construção (`VENTISEG_ENABLE_GPIO=0`). Nenhuma gravação com GPIO ativo foi
feita.

## Confirmado nesta entrega

- Perfil `ventiseg` registrado no `tools/flash.ps1` e na tabela de perfis.
- Pinagem STEP=IO3, DIR=IO21, IO45/46 reservados por strapping.
- Driver DM556D (Azaneli/Policomp/Tecmaf), 24–50 VDC, 2,1–5,6 A, alimentado
  a 24–36 VDC / mín. 4 A.
- Motor **KTC-HT23-400**, 8 fios, bipolar paralela 2,8 A, 1,9 Nm; fios
  mapeados para `A±/B±` e DIP SW1–SW4 definidos.
- Interface em **catodo comum** com conversor obrigatório **3,3 V → 5 V**
  (optos exigem alto > 3,5 V); `ENA` desconectado, driver inicia habilitado.
- A **SIMCAM detecta a pessoa e comanda o pan** — visão e controle na mesma
  placa, sem controladora externa (reafirmado pelo usuário em 20/11/2026).
- **A câmera gira junto com o duto**: malha fechada; a visão compensa o
  próprio movimento do pan.
- Protocolo v0.1 especificado; `T` é o resultado da detecção interna.
  Porta UDP 4211 apenas reservada para depuração futura; serial-only agora.
- Guia executivo de ligação em `docs/LIGACOES.md` e esquema WireViz em nível
  de terminal (`PUL±/DIR±/ENA±`, fases `A±/B±`, conversor 3,3→5 V, 24 V):
  **variante catodo comum desenhada**, validada pelo WireViz 0.4.1.
- Núcleo TDD entregue: `tools/protocol_ref.py` → `controller/parser.cpp` →
  52 vetores gerados → `autotest.h` → simulador, com **139 testes host
  verdes** (111 protocolo + 28 simulador). Watchdog renovado **somente** por
  `T` aceito no firmware e no simulador (divergência corrigida).
- Divergências do simulador corrigidas em 21/11/2026 para espelhar Python/C++:
  teto de `seq` = `INT32_MAX` (antes aceitava até 2^53), idade `rx_ms`/`now_ms`
  aplicada a qualquer comando (antes inexistente), zeros à esquerda aceitos e
  rejeição por faixa ligando `fault=Range` (antes só por injeção na UI).
  `PROTOCOLO.md` e o painel do simulador agora documentam a ordem completa
  `tamanho/codificação → formato → faixa → sequência → idade`.
- Sketch compila na FQBN `ventiseg` (313.540 B flash / 22.960 B RAM), com
  `VENTISEG_ENABLE_GPIO=0` e nenhuma chamada de GPIO/Wi-Fi.

## Ainda não feito

- Autoteste **na placa** não executado: nenhuma gravação foi feita nesta
  entrega. Detectar a porta na hora antes de qualquer upload.
- `VENTISEG_STEPS_PER_REV` segue **3200 (provisório)**. O alvo é 1600
  (1/8 passo) assim que SW5–SW8 forem lidos na carcaça; a troca exige
  regenerar vetores e rodar a suíte no mesmo commit.

## Pendências antes de qualquer ensaio com motor

| ID | Pendência | Bloqueia |
|---|---|---|
| P1 | Corrente do opto, pulso mínimo e origem do 5 V do conversor (topologia e ENA já fechados) | Energização do sinal |
| P2 | Posições exatas de SW5–SW8 e curso útil/batentes (modelo, fios e corrente já fechados) | Movimento |
| P3 | Conferir potência da fonte, fusível, bitolas e aterramento | Energização |
| P4 | Acoplamento motor↔duto e limite por software | Movimento |
| P5 | Observar pulso no boot/reset e travamento com carga desacoplada | Segurança |

## Regra de atualização

Toda mudança de firmware atualiza este arquivo, `docs/PROTOCOLO.md`,
`docs/HARDWARE-DECISIONS.md` e `docs/DEVLOG.md` da raiz no mesmo commit.
