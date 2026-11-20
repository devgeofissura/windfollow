# Guia de ligação — ventiseg (documentação + execução)

Complemento executável de [`wireviz/ventiseg-pan-24v.yaml`](wireviz/ventiseg-pan-24v.yaml).
Aqui estão os **terminais exatos** de cada ponta. Este arquivo manda montar;
[`HARDWARE-DECISIONS.md`](HARDWARE-DECISIONS.md) registra o que é fato e o que é
`NAO CONFIRMADO`.

> **ANTES DE LIGAR:** `VENTISEG_ENABLE_GPIO=0` ⇒ nenhum GPIO é escrito. Não
> energize o motor, não acople a carga e não pule P1–P5 de
> [`ESTADO.md`](ESTADO.md). **Nunca ligue um GPIO de 3,3 V direto em PUL/DIR**
> do DM556D: a entrada é optoacoplada.

## 1. Lado da SIMCAM — CONFIRMADO (serigrafia `J_GPIO_LIVRES`)

| Pino do conector | GPIO | Uso no ventiseg | Vai para |
|---|---|---|---|
| 1 | IO3 / GPIO3 | `STEP` (PUL) | interface de sinal, entrada `STEP_IN` |
| 2 | IO46 / GPIO46 | **reservado** | nada — strapping, não puxar HIGH no boot |
| 3 | IO45 / GPIO45 | **reservado** | nada — strapping, define tensão da Flash |
| 4 | IO21 / GPIO21 | `DIR` | interface de sinal, entrada `DIR_IN` |
| 5 | GND | referência lógica | `GND_CTRL` da interface **e** ponto único de terra |
| 6 | 3V3 | sensores apenas | **NÃO usar** no driver nem no motor |

## 2. Lado do DM556D (Azaneli) — terminais da etiqueta

Terminais conferidos no protótipo do `laserseg`; **confira os do seu exemplar**
antes de ligar (P1).

| Terminal | Nome | Ligação no ventiseg |
|---|---|---|
| `PUL+` / `PUL-` | passos | saída da interface (polaridade conforme §3) |
| `DIR+` / `DIR-` | direção | saída da interface (polaridade conforme §3) |
| `ENA+` / `ENA-` | habilitação | **deixar sem ligação** somente se o manual disser que o driver habilita solto; nunca ligar por suposição |
| `A+` / `A-` | fase A do motor | uma das duas bobinas do motor (§4) |
| `B+` / `B-` | fase B do motor | a outra bobina do motor (§4) |
| `+Vdc` | alimentação | **+24 V** da fonte externa |
| `GND` | alimentação | 0 V da fonte externa **e** terra comum (§5) |

## 3. Interface obrigatória 3,3 V → opto (P1)

O GPIO da ESP32-S3 **não alimenta o opto diretamente**. Use um conversor/
buffer com saída em coletor aberto (ou circuito equivalente) dimensionado para
a corrente do opto do seu DM556D. Existem duas variantes:

| Variante | Comum | Sinal comandado | Quando usar |
|---|---|---|---|
| **A — comum-ânodo** (desenhada no YAML) | `+V_OPTO` → `PUL+` e `DIR+` | a interface **afunda** `PUL-` e `DIR-` | o usual em DM556/DM556D; confirme no manual |
| **B — comum-cátodo** | `PUL-` e `DIR-` → `GND` | a interface **aciona** `PUL+` e `DIR+` | se o manual/etiqueta do seu exemplar indicar |

`NAO CONFIRMADO` e obrigatório antes de soldar: tensão do comum do opto
(pode ser 3,3 V ou 5 V conforme a unidade), corrente do LED do opto, pulso
mínimo e lógica do pino `ENA`. Variantes de DM556 citam 7–16 mA e pulso
≥ 2,5 µs, mas isso **não prova** o exemplar Azaneli.

Se a sua unidade for **comum-cátodo**, espelhe o YAML: o comum vai em
`PUL-`/`DIR-` e os sinais em `PUL+`/`DIR+`. Não altere só o desenho sem
alterar a montagem.

## 4. Motor — ligação das fases (P2/P4)

1. Motor **desenergizado e desacoplado** do duto.
2. Com um ohmímetro, ache os **dois pares de fios com continuidade entre si**.
   Cada par é uma bobina. **Não escolha pela cor dos fios.**
3. Um par vai em `A+`/`A-`; o outro em `B+`/`B-`. A ordem dentro do par define
   o sentido de rotação.
4. Se ao ensaiar o sentido estiver invertido, troque `A+`↔`A-` **ou**
   `B+`↔`B-` com a fonte desligada. Nunca troque só um fio entre fases.
5. Ajuste a corrente do driver pela etiqueta do motor e pelos DIP switches do
   seu exemplar. Os valores do `laserseg` (3200 passos/volta, SW5–SW8
   `OFF OFF ON ON`) são **provisórios e NAO CONFIRMADOS** aqui.

`NAO CONFIRMADO`: modelo, corrente/fase, torque, tamanho e curso útil.

## 5. Alimentação e terra comum (P3)

```
Fonte 24 VDC  +24 V ----------> DM556D +Vdc
Fonte 24 VDC  0 V  ----------> DM556D GND
                                  |
GND da SIMCAM -------------------+----> ponto único (star ground)
```

- A fonte é **exclusiva** do driver/motor. Nunca alimente motor ou driver pelo
  `3V3` ou USB da SIMCAM.
- GND lógico, GND do driver e 0 V da fonte se encontram em **um único ponto**
  (representado por `J_REF_GND` no YAML).
- `NAO CONFIRMADO`: corrente/potência da fonte, fusível, bitola dos cabos e
  dissipação do driver.

## 6. Ordem de montagem e ensaio

| # | Ação | Critério para seguir |
|---|---|---|
| 1 | Ler manual/foto do DM556D Azaneli | comum do opto, corrente, pulso mínimo, ENA e DIP confirmados (fecha P1) |
| 2 | Ler etiqueta do motor e ajustar corrente | corrente/fase compatível com o driver (fecha P2) |
| 3 | Conferir fonte 24 V, fusível e bitolas | dimensionamento fechado (fecha P3) |
| 4 | Montar com o motor **desacoplado** do duto | sem carga mecânica |
| 5 | Medir a saída da interface (LED/osciloscópio) | pulso presente e com a polaridade da variante escolhida |
| 6 | Definir curso em passos e comportamento no batente | limite por software definido (fecha P4) |
| 7 | Observar boot/reset do driver com carga desacoplada | sem pulso falso no motor (fecha P5) |
| 8 | Só então acoplar ao duto e calibrar ganho/curso | `VENTISEG_ENABLE_GPIO=1` registrado em decisão |

`VENTISEG_ENABLE_GPIO=0` continua travado até o item 7.
