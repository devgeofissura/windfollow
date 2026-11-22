Abaixo está o **mapa par a par** e a **ordem física de montagem** do ventiseg, tirado de `docs/LIGACOES.md`, `docs/HARDWARE-DECISIONS.md` e do esquema WireViz `docs/wireviz/ventiseg-pan-24v.yaml`.

> ⚠️ **Estado atual:** auditoria P1–P5 **aberta**. Firmware em `VENTISEG_ENABLE_GPIO=0`. Isto é o plano de ligação **documentado**, não autorização para energizar. Motor desacoplado, fonte desligada, Conversor/5 V ainda não conferidos no exemplar.

---

## 0. Peças envolvidas (pares que aparecem abaixo)

| # | Hardware | Papel | Status |
|---|---|---|---|
| H1 | **ESP32-S3 LILYGO T-SIMCAM** | detecta a pessoa, gera alvo, emite STEP/DIR | confirmado |
| H2 | **Conversor de nível 3,3 ↔ 5 V** (2 canais) | adapta GPIO para os optos | obrigatório, modelo pendente (P1) |
| H3 | **Fonte auxiliar 5 V** (DC-DC 24→5 V proposto) | alimenta só o lado HV do conversor | **não confirmado** (P1) |
| H4 | **DM556D** (Azaneli/Policomp/Tecmaf) | driver do eixo único | confirmado |
| H5 | **Motor KTC-HT23-400**, 8 fios, 2,8 A, 1,9 Nm | gira duto + câmera | confirmado |
| H6 | **Fonte 24 VDC** (proposta 6,25 A/150 W) | exclusiva do driver/motor | proposta não conferida (P3) |
| H7 | **Ponto único de terra** (`J_REF_GND`, star ground) | referência comum | topologia documentada |
| H8 | **Conjunto duto + câmera** | carga mecânica / malha fechada | acoplamento pendente (P4) |

---

## 1. PAR: SIMCAM ↔ conversor de nível (lado LV, 3,3 V)

Conector preto `J_GPIO_LIVRES` da SIMCAM (serigrafia do usuário = fonte primária):

| Pino SIMCAM | Sinal | Vai para o conversor | Cor (YAML) |
|---|---|---|---|
| 1 | `IO3 / GPIO3` = **STEP** | `STEP_LV` | amarelo |
| 4 | `IO21 / GPIO21` = **DIR** | `DIR_LV` | cinza |
| 6 | `3V3` | `VCC_3V3` | vermelho |
| 5 | `GND` | `GND_LV` | preto |
| 2 | `IO46` | **nada** — strapping, não puxar HIGH no boot | — |
| 3 | `IO45` | **nada** — strapping, define tensão da Flash no boot | — |

- Fios de sinal: 0,25 mm², par trançado blindado, afastados de potência e fases do motor.
- **Nunca** ligar 5 V nos pinos da SIMCAM.
- A skill `espressif-lilygo` lista IO45/46 como modem e IO21 como status; para o ventiseg **prevalece a serigrafia da bancada**: IO3 = STEP, IO21 = DIR.

## 2. PAR: fonte 5 V ↔ conversor (lado HV)

| Fonte 5 V (H3) | Conversor HV |
|---|---|
| `+5 V` (laranja) | `VCC_5V` |
| `GND_5V` (preto) | `GND_HV` → mesmo star ground |

- O 5 V **não** vem do `3V3` da SIMCAM.
- Manual Policomp/Tecmaf consultado **não documenta saída auxiliar de 5 V** no DM556D: proposta é DC-DC externo 24 V → 5 V, **≥100 mA** (200 mA dá margem).
- **P1:** conferir datasheet do conversor 3,3↔5 V, corrente real do opto (referência 8–20 mA, típ. 10 mA), pulso mínimo e **medir 5 V antes de ligar `PUL+`/`DIR+`**.

## 3. PAR: conversor ↔ DM556D (entrada de sinal — catodo comum)

| Conversor HV | DM556D | Cor |
|---|---|---|
| `STEP_HV` | `PUL+` | marrom |
| `DIR_HV` | `DIR+` | azul |
| — | `PUL-` | **GND da SIMCAM** (pino 5) — preto |
| — | `DIR-` | **GND da SIMCAM** (pino 5) — preto |
| — | `ENA+` / `ENA-` | **desconectados** (driver já inicia habilitado) |

- Topologia desenhada e decidida: **catodo comum**. Se a etiqueta do seu exemplar mostrar outra topologia, **pare** e atualize o esquema.
- Os optos exigem alto **> 3,5 V** — por isso o conversor é obrigatório; GPIO de 3,3 V direto em `PUL+`/`DIR+` não serve.

## 4. PAR: fonte 24 V ↔ DM556D (potência)

| Fonte 24 V (H6) | DM556D | Observação |
|---|---|---|
| `+V` (vermelho, 1,5 mm² provisório) | `VCC / +Vdc` | 24–36 V, mín. 4 A; driver aceita 24–50 V |
| `0 V` (preto, 1,5 mm² provisório) | `GND` de potência | mesmo star ground |

- Fonte **exclusiva** do driver/motor. Nunca alimentar pelo USB/3V3 da SIMCAM.
- **Nunca inverter polaridade.**
- Proposta P3: fusível DC **7,5 A retardado** perto do `+`; bitolas, dissipação e curva do fusível **não conferidas**.

## 5. PAR: DM556D ↔ motor KTC-HT23-400 (bipolar paralela)

Um borne do driver recebe **dois fios da mesma bobina**:

| Borne driver | Fios do motor |
|---|---|
| `A+` | **Vermelho** + **Preto/Branco** |
| `A-` | **Preto** + **Vermelho/Branco** |
| `B+` | **Verde** + **Amarelo/Branco** |
| `B-` | **Amarelo** + **Verde/Branco** |

- Antes: motor **desenergizado e desacoplado**; confirmar continuidade dos pares com ohmímetro (a cor é referência, o ohmímetro é a prova).
- DIP: SW1/SW2/SW3 = `OFF/ON/OFF` (2,7–2,8 A); SW4 = `OFF` (meia corrente em repouso).
- SW5–SW8: alvo **1600 pulsos/volta = `ON/OFF/ON/ON`**; o firmware segue **3200 provisório**. **P2:** ler a carcaça antes de trocar; trocar exige regenerar vetores e rodar a suíte no mesmo commit.
- Se o sentido estiver invertido no ensaio, troque `A+↔A-` **ou** `B+↔B-` com a fonte desligada. Nunca troque só um fio entre fases.

## 6. PAR: todos os GNDs ↔ ponto único (star ground)

Quatro braços chegam ao mesmo nó `J_REF_GND`:

1. `GND` da SIMCAM (lógico) — também alimenta `PUL-`/`DIR-`;
2. `GND` da fonte 5 V / lado HV do conversor;
3. `GND` de potência do DM556D;
4. `0 V` da fonte 24 V.

- PE vai ao **chassi**, nunca ao retorno de corrente.
- **P3:** continuidade real do star ground não foi medida.

## 7. PAR: motor ↔ duto + câmera (mecânico)

- Um único motor gira o conjunto **câmera + duto/ventilador**: a SIMCAM viaja junto, então a visão é **malha fechada** (o erro compensa o próprio pan).
- Sem fim de curso, relé, laser, iluminador nem eixo de tilt.
- **P4:** acoplamento, curso útil, batentes e referência/home **não medidos**; `kMaxTravel=533` é provisório.
- A cableagem da SIMCAM no conjunto girante precisa de folga para não puxar conector nem travar rotação.

## 8. PAR: SIMCAM ↔ PC (somente diagnóstico)

- USB apenas para serial/boot/telemetria (115200). **Não** faz parte do laço de controle e **nunca** alimenta motor/driver.
- Porta muda entre sessões: detectar na hora com `tools/flash.ps1 -Sketch ventiseg -DetectOnly`.

---

## Ordem física de montagem (checklist)

| # | Ação | Critério para avançar |
|---|---|---|
| 1 | Ler etiqueta/manual do DM556D | catodo comum, corrente do opto, pulso mínimo, origem do 5 V → fecha **P1** |
| 2 | Conferir 8 fios do motor com ohmímetro e ajustar DIP | paralela correta, 2,8 A → fecha **P2** |
| 3 | Conferir fonte 24 V, fusível, bitolas, star ground | dimensionamento fechado → fecha **P3** |
| 4 | Montar sinais: SIMCAM → conversor → `PUL+`/`DIR+`, GND em `PUL-`/`DIR-`, ENA solto | 5 V medido antes de ligar optos |
| 5 | Montar potência: fonte → driver → motor, **motor desacoplado** | sem carga mecânica |
| 6 | Medir `PUL+`/`DIR+` com osciloscópio | pulsos de 5 V na polaridade do catodo comum → fecha parte de **P5** |
| 7 | Medir curso até batentes e definir limite por software | fecha **P4** |
| 8 | Observar boot/reset do driver com carga desacoplada | sem pulso falso → fecha **P5** |
| 9 | Só então acoplar ao duto/câmera e calibrar ganho/curso | registrar `VENTISEG_ENABLE_GPIO=1` em decisão |

**Enquanto os itens 1–8 não estiverem medidos e registrados:** não energizar driver/motor, não liberar os gates de compilação e não integrar captura de câmera real. Nenhuma gravação nova foi feita; esta resposta é apenas documentação dos pares.
