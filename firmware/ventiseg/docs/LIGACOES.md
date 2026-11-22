# Guia de ligação — ventiseg (documentação + execução)

Complemento executável de [`wireviz/ventiseg-pan-24v.yaml`](wireviz/ventiseg-pan-24v.yaml).
Aqui estão os **terminais exatos** de cada ponta. Este arquivo manda montar;
[`HARDWARE-DECISIONS.md`](HARDWARE-DECISIONS.md) registra o que é fato e o que é
`NAO CONFIRMADO`.

> **ANTES DE LIGAR:** `VENTISEG_ENABLE_GPIO=0` ⇒ nenhum GPIO é escrito. Não
> energize o motor, não acople a carga e não pule P1–P5 de
> [`ESTADO.md`](ESTADO.md). Desde 22/11/2026 a interface é **ULN2003A em anodo
> comum**: o +5 V externo vai em `PUL+`/`DIR+` e as saídas `A`/`B` drenam
> `PUL-`/`DIR-`. **Nunca ligue o GPIO direto nos optos** e nunca tire o 5 V do
> `3V3` da SIMCAM.

## 1. Lado da SIMCAM — CONFIRMADO (serigrafia `J_GPIO_LIVRES`)

| Pino do conector | GPIO | Uso no ventiseg | Vai para |
|---|---|---|---|
| 1 | IO3 / GPIO3 | `STEP` (PUL) | `In1` do módulo ULN2003A |
| 2 | IO46 / GPIO46 | **reservado** | nada — strapping, não puxar HIGH no boot |
| 3 | IO45 / GPIO45 | **reservado** | nada — strapping, define tensão da Flash |
| 4 | IO21 / GPIO21 | `DIR` | `In2` do módulo ULN2003A |
| 5 | GND | referência lógica | pino `GND` do ULN e ponto único de terra |
| 6 | 3V3 | **livre nesta ligação** | o ULN2003A não usa 3V3; não ir ao driver |

> A troca `IO21→PUL` / `IO3→DIR` que apareceu em uma mensagem foi tratada como
> typo: prevalece **IO3 = STEP/PUL** e **IO21 = DIR**, conforme a serigrafia.

## 2. Lado do DM556D (Policomp/Tecmaf) — terminais da etiqueta

| Terminal | Nome | Ligação no ventiseg |
|---|---|---|
| `PUL+` | passo (ânodo do opto) | **+5 V** da fonte auxiliar |
| `PUL-` | passo (cátodo do opto) | saída `A` do ULN2003A |
| `DIR+` | direção (ânodo do opto) | **+5 V** da fonte auxiliar |
| `DIR-` | direção (cátodo do opto) | saída `B` do ULN2003A |
| `ENA+` / `ENA-` | habilitação | **desconectados** — o driver já inicia habilitado por padrão |
| `A+` / `A-` | fase A do motor | dois fios da bobina A em paralelo (§4) |
| `B+` / `B-` | fase B do motor | dois fios da bobina B em paralelo (§4) |
| `VCC` / `+Vdc` | alimentação | **+24 a +36 V** da fonte externa (mín. 4 A) |
| `GND` | alimentação | 0 V da fonte externa **e** terra comum (§5) |

**Topologia: ANODO COMUM.** É a variante decidida pelo usuário em 22/11/2026 e
desenhada nos YAMLs do WireViz. `PUL+`/`DIR+` recebem +5 V fixo; `PUL-`/`DIR-`
são comandados pelo ULN. Se o seu exemplar mostrar outra topologia na etiqueta,
**pare** e atualize o esquema antes de soldar.

## 3. Módulo ULN2003A — interface de sinal (22/11/2026)

O GPIO da ESP32-S3 **não alimenta o opto diretamente**. Use o módulo
**ULN2003A com barra de pinos completa** informado pelo usuário (entradas
`In1`–`In7` à esquerda, saídas `A`–`G` à direita). É um driver de corrente de
coletor aberto: cada canal drena até 500 mA — folga para os ~10 mA do opto do
DM556D, sem carregar a SIMCAM.

| Pino do módulo ULN2003A | Origem/destino | Função |
|---|---|---|
| `In1` | IO3 da SIMCAM (`STEP`) | recebe o pulso de 3,3 V |
| `In2` | IO21 da SIMCAM (`DIR`) | recebe a direção de 3,3 V |
| `GND` | pino 5 da SIMCAM + star ground | terra comum de sinal, 5 V, driver e fonte 24 V |
| `VCC` | +5 V da fonte auxiliar | jumper `<->` fechado para os LEDs D1/D2 indicarem os sinais |
| `A` | `PUL-` do DM556D | drena o cátodo do opto de STEP |
| `B` | `DIR-` do DM556D | drena o cátodo do opto de DIR |
| `C`–`G` | não conectados | canais livres |

- O +5 V **não** sai do `3V3` da SIMCAM. A documentação Policomp/Tecmaf
  consultada **não** mostra saída auxiliar de 5 V no DM556D: use **DC-DC
  externo 24 V → 5 V**, com capacidade ≥100 mA (200 mA dá margem), e meça a
  saída antes de conectar `PUL+`/`DIR+`.
- Os LEDs D1/D2 do módulo acendem junto com `STEP` e `DIR`: são indicadores
  visuais de bancada, não parte do laço de controle.
- **Atenção à inversão:** `In1`/`In2` em ALTO ligam `A`/`B` ao GND, então o
  opto conduz com o sinal invertido em relação ao GPIO. A borda reconhecida
  como `STEP` e o sentido de `DIR` precisam ser conferidos no osciloscópio e
  no ensaio com motor desacoplado. Se `DIR` estiver invertido, registrar a
  correção em `HARDWARE-DECISIONS.md` e ajustar o firmware com o gate fechado.
- `NAO CONFIRMADO` e obrigatório antes de soldar: datasheet do módulo ULN2003A
  (corrente de entrada a 3,3 V e jumper dos LEDs), corrente real do opto no
  exemplar (referência: 8–20 mA) e pulso mínimo do DM556D. P1 permanece aberta.

## 4. Motor KTC-HT23-400 — bipolar paralela (P2)

Motor de 8 fios, **2,8 A** em paralela, **1,9 Nm**. O DM556D aceita 2,1–5,6 A.

| Borne do driver | Fios do motor (juntos no mesmo borne) |
|---|---|
| `A+` | **Vermelho** + **Preto/Branco** |
| `A-` | **Preto** + **Vermelho/Branco** |
| `B+` | **Verde** + **Amarelo/Branco** |
| `B-` | **Amarelo** + **Verde/Branco** |

1. Motor **desenergizado e desacoplado** do duto.
2. Confirme com datasheet/ohmímetro que os pares acima têm continuidade —
   a cor é a referência informada, o ohmímetro é a prova.
3. Se ao ensaiar o sentido estiver invertido, troque `A+`↔`A-` **ou**
   `B+`↔`B-` com a fonte desligada. Nunca troque só um fio entre fases.

### DIP switches do DM556D

| Chaves | Posição | Efeito |
|---|---|---|
| SW1, SW2, SW3 | `OFF, ON, OFF` | corrente de 2,7 / 2,8 A (paralela) |
| SW4 | `OFF` | reduz pela metade a corrente em repouso |
| SW5–SW8 | 1/8 de passo → **1600 pulsos/volta** | `ON/OFF/ON/ON` (3200 = `OFF/OFF/ON/ON`); conferir na carcaça do exemplar (P2) |

> O firmware mantém `VENTISEG_STEPS_PER_REV = 3200` como valor **provisório**.
> O alvo é 1600 pulsos/volta; mudar a constante exige regenerar os vetores de
> autoteste e rodar a suíte host no mesmo commit. Não faça isso no escuro.

## 5. Alimentação e terra comum (P3)

```
Fonte 24-36 VDC, min. 4 A   +V ----------> DM556D VCC
Fonte 24-36 VDC, min. 4 A  0 V  ----------> DM556D GND
                                   |
GND da SIMCAM / pino GND do ULN ---+----> ponto único (star ground)
GND da fonte 5 V ------------------+----> mesmo ponto
```

- A fonte é **exclusiva** do driver/motor. Nunca alimente motor ou driver pelo
  `3V3` ou USB da SIMCAM. **Nunca inverta a polaridade.**
- GND lógico, GND do ULN/5 V, GND do driver e 0 V da fonte se encontram
  em **um único ponto** (representado por `J_REF_GND` no YAML). PE vai ao
  chassi, nunca ao retorno de corrente.
- **Proposta de dimensionamento (P3, ainda não conferida):** fonte 24 VDC
  regulada 6,25 A / 150 W; fusível DC 7,5 A retardado perto do `+`; 1,5 mm²
  fonte→driver; 1,0–1,5 mm² driver→motor; par trançado blindado 0,22–0,34 mm²
  para `PUL`/`DIR`/GND, afastado de potência e fases do motor.
- `NAO CONFIRMADO`: componentes reais, comprimentos, queda de tensão,
  dissipação, curva do fusível e continuidade do star ground. P3 aberta.

## 6. Ordem de montagem e ensaio

| # | Ação | Critério para seguir |
|---|---|---|
| 1 | Conferir manual/etiqueta do DM556D e o módulo ULN2003A | anodo comum, corrente do opto, jumper dos LEDs, pulso mínimo e origem do 5 V (fecha P1) |
| 2 | Conferir fios do KTC-HT23-400 e ajustar DIP | paralela correta e 2,8 A (fecha P2) |
| 3 | Conferir fonte 24–36 V / 4 A, fusível e bitolas | dimensionamento fechado (fecha P3) |
| 4 | Montar sinais: `In1`/`In2` ← SIMCAM, `VCC` ← 5 V, `A`/`B` → `PUL-`/`DIR-`, `+5 V` → `PUL+`/`DIR+`, `ENA` solto | 5 V medido antes de ligar optos |
| 5 | Montar potência e manter o motor **desacoplado** do duto | sem carga mecânica |
| 6 | Medir `PUL-`/`DIR-` com osciloscópio e conferir `+5 V` em `PUL+`/`DIR+` | afundamento pelo ULN e polaridade de anodo comum (parte de P5) |
| 7 | Conferir borda de `STEP` e sentido de `DIR` depois da inversão do ULN | inversão registrada/corrigida em decisão; gate ainda fechado (fecha P5) |
| 8 | Definir curso em passos e comportamento no batente | limite por software definido (fecha P4) |
| 9 | Observar boot/reset do driver com carga desacoplada | sem pulso falso no motor (fecha P5) |
| 10 | Só então acoplar ao duto e calibrar ganho/curso | `VENTISEG_ENABLE_GPIO=1` registrado em decisão |

`VENTISEG_ENABLE_GPIO=0` continua travado até os itens 1–9.
