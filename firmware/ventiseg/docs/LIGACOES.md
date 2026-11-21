# Guia de ligação — ventiseg (documentação + execução)

Complemento executável de [`wireviz/ventiseg-pan-24v.yaml`](wireviz/ventiseg-pan-24v.yaml).
Aqui estão os **terminais exatos** de cada ponta. Este arquivo manda montar;
[`HARDWARE-DECISIONS.md`](HARDWARE-DECISIONS.md) registra o que é fato e o que é
`NAO CONFIRMADO`.

> **ANTES DE LIGAR:** `VENTISEG_ENABLE_GPIO=0` ⇒ nenhum GPIO é escrito. Não
> energize o motor, não acople a carga e não pule P1–P5 de
> [`ESTADO.md`](ESTADO.md). **Nunca ligue um GPIO de 3,3 V direto em PUL+/DIR+**
> do DM556D: os optos exigem nível alto acima de 3,5 V. Use o conversor.

## 1. Lado da SIMCAM — CONFIRMADO (serigrafia `J_GPIO_LIVRES`)

| Pino do conector | GPIO | Uso no ventiseg | Vai para |
|---|---|---|---|
| 1 | IO3 / GPIO3 | `STEP` (PUL) | conversor de nível, entrada `STEP_LV` |
| 2 | IO46 / GPIO46 | **reservado** | nada — strapping, não puxar HIGH no boot |
| 3 | IO45 / GPIO45 | **reservado** | nada — strapping, define tensão da Flash |
| 4 | IO21 / GPIO21 | `DIR` | conversor de nível, entrada `DIR_LV` |
| 5 | GND | referência lógica | `GND_LV`, `PUL-`, `DIR-` e ponto único de terra |
| 6 | 3V3 | lógica de baixa corrente | `VCC_3V3` do conversor — **não** ao driver |

> A troca `IO21→PUL` / `IO3→DIR` que apareceu em uma mensagem foi tratada como
> typo: prevalece **IO3 = STEP/PUL** e **IO21 = DIR**, conforme a serigrafia.

## 2. Lado do DM556D (Policomp/Tecmaf) — terminais da etiqueta

| Terminal | Nome | Ligação no ventiseg |
|---|---|---|
| `PUL+` | passo (ânodo do opto) | saída `STEP_HV` do conversor, **5 V** |
| `PUL-` | passo (cátodo do opto) | **GND da SIMCAM** |
| `DIR+` | direção (ânodo do opto) | saída `DIR_HV` do conversor, **5 V** |
| `DIR-` | direção (cátodo do opto) | **GND da SIMCAM** |
| `ENA+` / `ENA-` | habilitação | **desconectados** — o driver já inicia habilitado por padrão |
| `A+` / `A-` | fase A do motor | dois fios da bobina A em paralelo (§4) |
| `B+` / `B-` | fase B do motor | dois fios da bobina B em paralelo (§4) |
| `VCC` / `+Vdc` | alimentação | **+24 a +36 V** da fonte externa (mín. 4 A) |
| `GND` | alimentação | 0 V da fonte externa **e** terra comum (§5) |

**Topologia: CATODO COMUM.** É a variante desenhada no YAML e confirmada para
esta bancada. Se o seu exemplar mostrar outra topologia na etiqueta, **pare** e
atualize o esquema antes de soldar.

## 3. Conversor de nível obrigatório 3,3 V → 5 V

O GPIO da ESP32-S3 **não alimenta o opto diretamente**. Use um conversor
bidirecional de 2 canais (3,3 V ↔ 5 V) ou driver de linha equivalente:

| Lado conversor | Pino | Origem/destino |
|---|---|---|
| LV (3,3 V) | `STEP_LV` | IO3 da SIMCAM |
| LV (3,3 V) | `DIR_LV` | IO21 da SIMCAM |
| LV (3,3 V) | `VCC_3V3` | pino 6 (`3V3`) da SIMCAM |
| LV (3,3 V) | `GND_LV` | pino 5 (GND) da SIMCAM |
| HV (5 V) | `STEP_HV` | `PUL+` do DM556D |
| HV (5 V) | `DIR_HV` | `DIR+` do DM556D |
| HV (5 V) | `VCC_5V` | fonte 5 V — origem a confirmar (P1) |
| HV (5 V) | `GND_HV` | mesmo ponto estrela dos demais GND |

- O 5 V **não** pode sair do `3V3` da SIMCAM. A documentação Policomp/Tecmaf
  consultada **não** mostra saída auxiliar de 5 V no DM556D: use **DC-DC
  externo 24 V → 5 V**, com capacidade ≥100 mA (200 mA dá margem), e meça a
  saída antes de conectar `PUL+`/`DIR+`.
- `NAO CONFIRMADO` e obrigatório antes de soldar: modelo do conversor
  3,3↔5 V, corrente real do opto no exemplar (referência: 8–20 mA) e pulso
  mínimo do DM556D. Não inventar esses valores; P1 permanece aberta.

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
GND da SIMCAM -------------------+----> ponto único (star ground)
GND da fonte 5 V ----------------+----> mesmo ponto
```

- A fonte é **exclusiva** do driver/motor. Nunca alimente motor ou driver pelo
  `3V3` ou USB da SIMCAM. **Nunca inverta a polaridade.**
- GND lógico, GND do conversor/5 V, GND do driver e 0 V da fonte se encontram
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
| 1 | Conferir manual/etiqueta do DM556D | catodo comum, corrente do opto, pulso mínimo e origem do 5 V (fecha P1) |
| 2 | Conferir fios do KTC-HT23-400 e ajustar DIP | paralela correta e 2,8 A (fecha P2) |
| 3 | Conferir fonte 24–36 V / 4 A, fusível e bitolas | dimensionamento fechado (fecha P3) |
| 4 | Montar com o motor **desacoplado** do duto | sem carga mecânica |
| 5 | Medir `PUL+`/`DIR+` com osciloscópio | pulsos de 5 V na polaridade do catodo comum |
| 6 | Definir curso em passos e comportamento no batente | limite por software definido (fecha P4) |
| 7 | Observar boot/reset do driver com carga desacoplada | sem pulso falso no motor (fecha P5) |
| 8 | Só então acoplar ao duto e calibrar ganho/curso | `VENTISEG_ENABLE_GPIO=1` registrado em decisão |

`VENTISEG_ENABLE_GPIO=0` continua travado até o item 7.
