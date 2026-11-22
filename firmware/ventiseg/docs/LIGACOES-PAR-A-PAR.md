Abaixo está o **mapa par a par** e a **ordem física de montagem** do ventiseg, consolidado em 22/11/2026 a partir de `AGENTS.md`, `docs/LIGACOES.md`, `docs/HARDWARE-DECISIONS.md` e do esquema WireViz `docs/wireviz/ventiseg-pan-24v.yaml`. A interface de sinal passou do conversor bidirecional para **módulo ULN2003A em anodo comum** (decisão do usuário em 22/11/2026).

> ⚠️ **Estado atual:** auditoria P1–P5 **aberta**. Firmware em `VENTISEG_ENABLE_GPIO=0`. Isto é o plano de ligação **documentado**, não autorização para energizar. Motor desacoplado, fonte desligada, ULN2003A/5 V ainda não medidos no exemplar.

---

## Dados informados pelo usuário (22/11/2026) — registrados textualmente

- Vou usar esta placa para conversor de nível: **é exatamente o módulo ULN2003A com barra de pinos completa (entradas In1 a In7 na esquerda e saídas A a G na direita)**.
- Diferente da plaquinha anterior, esse chip possui **transistores Darlington internos que drenam até 500 mA por canal** — sobra corrente para acionar os **optoacopladores de 10 mA do DM556D** com segurança e sem sobrecarregar a SIMCAM.
- Como esta versão tem a **barra de pinos preta na direita (A a G)**, não é preciso adaptar nada no conector branco: **basta encaixar jumpers fêmea direto nos pinos**.
- Mapa exato da placa da foto:
  - `In1` (esquerda) ← **GPIO3 da SIMCAM (STEP)** — recebe o pulso de 3,3 V;
  - `In2` (esquerda) ← **GPIO21 da SIMCAM (DIR)** — recebe a direção de 3,3 V;
  - `GND` (inferior) ← **GND comum (star ground)** — SIMCAM + fonte 5 V;
  - `VCC` (inferior) ← **+5 V da fonte auxiliar** — mantém o jumper `<->` fechado para os LEDs D1/D2 funcionarem;
  - `A` (direita, topo) → **PUL- do DM556D** — saída correspondente ao `In1`;
  - `B` (direita, 2º pino) → **DIR- do DM556D** — saída correspondente ao `In2`.
- **Topologia Anodo Comum:** ligar o **+5 V da fonte auxiliar diretamente nos pinos `PUL+` e `DIR+` do DM556D**.
- **Vantagem extra:** os **LEDs D1 e D2** dessa placa acendem junto com os sinais de **STEP** e **DIR**, servindo de indicador visual na bancada.
- **Atenção ao código/documentação:** como o **ULN2003 inverte o sinal** (nível **ALTO** na entrada `In1` liga o pino `A` ao terra), o esquema passa de **catodo comum** para **anodo comum** e deve-se **verificar no ensaio se o sentido do pino `DIR` ficou invertido**.

---

## 0. Peças envolvidas (pares que aparecem abaixo)

| # | Hardware | Papel | Status |
|---|---|---|---|
| H1 | **ESP32-S3 LILYGO T-SIMCAM** | detecta a pessoa, gera alvo, emite STEP/DIR | confirmado |
| H2 | **Módulo ULN2003A**, barra completa (`In1`–`In7` / `A`–`G`) | drena os optos; 500 mA/canal | decisão do usuário em 22/11/2026 (datasheet do módulo em P1) |
| H3 | **Fonte auxiliar 5 V** (DC-DC 24→5 V proposto) | alimenta `VCC` do ULN (LEDs D1/D2) e `PUL+`/`DIR+` do driver | **não confirmado** (P1) |
| H4 | **DM556D** (Azaneli/Policomp/Tecmaf) | driver do eixo único | confirmado |
| H5 | **Motor KTC-HT23-400**, 8 fios, 2,8 A, 1,9 Nm | gira duto + câmera | confirmado |
| H6 | **Fonte 24 VDC** (proposta 6,25 A/150 W) | exclusiva do driver/motor | proposta não conferida (P3) |
| H7 | **Ponto único de terra** (`J_REF_GND`, star ground) | referência comum | topologia documentada |
| H8 | **Conjunto duto + câmera** | carga mecânica / malha fechada | acoplamento pendente (P4) |

---

## 1. PAR: SIMCAM ↔ ULN2003A (entradas, lado 3,3 V)

Conector preto `J_GPIO_LIVRES` da SIMCAM (serigrafia do usuário = fonte primária). Não é preciso adaptar conector: encaixar jumpers fêmea direto na barra do módulo.

| Pino SIMCAM | Sinal | Pino do módulo ULN2003A | Cor (YAML) |
|---|---|---|---|
| 1 | `IO3 / GPIO3` = **STEP** | `In1` | amarelo |
| 4 | `IO21 / GPIO21` = **DIR** | `In2` | cinza |
| 5 | `GND` | `GND` (inferior) | preto |
| 6 | `3V3` | **não conectado** — o ULN não usa 3V3 | — |
| 2 | `IO46` | **nada** — strapping, não puxar HIGH no boot | — |
| 3 | `IO45` | **nada** — strapping, define tensão da Flash no boot | — |

- A GPIO de 3,3 V fornece apenas a corrente de entrada do Darlington; a corrente do LED do opto vem do +5 V externo. **Nunca** ligar 5 V nos pinos da SIMCAM.
- Fios de sinal: 0,25 mm², par trançado blindado, afastados de potência e fases do motor.
- Os canais `In3`–`In7` e as saídas `C`–`G` ficam **livres**.
- A skill `espressif-lilygo` lista IO45/46 como modem e IO21 como status; para o ventiseg **prevalece a serigrafia da bancada**: IO3 = STEP, IO21 = DIR.

## 2. PAR: fonte 5 V ↔ ULN2003A e DM556D

| Fonte 5 V (H3) | Módulo ULN2003A | DM556D | Cor |
|---|---|---|---|
| `+5 V` (laranja) | `VCC` (inferior) — jumper `<->` fechado, LEDs D1/D2 | `PUL+` **e** `DIR+` | laranja |
| `GND_5V` (preto) | — | mesmo star ground | preto |

- O 5 V **não** vem do `3V3` da SIMCAM.
- Manual Policomp/Tecmaf consultado **não documenta saída auxiliar de 5 V** no DM556D: proposta é DC-DC externo 24 V → 5 V, **≥100 mA** (200 mA dá margem).
- **P1:** conferir datasheet do módulo ULN2003A (entrada a 3,3 V, jumper dos LEDs), corrente real do opto (referência 8–20 mA, típ. 10 mA), pulso mínimo e **medir 5 V em `PUL+`/`DIR+` antes de ligar os optos**.

## 3. PAR: ULN2003A ↔ DM556D (entrada de sinal — ANODO COMUM)

| Módulo ULN2003A | DM556D | Cor | Efeito elétrico |
|---|---|---|---|
| `A` (direita, topo) | `PUL-` | preto | `In1` em ALTO liga `A` ao terra → opto de STEP conduz |
| `B` (direita, 2º pino) | `DIR-` | preto | `In2` em ALTO liga `B` ao terra → opto de DIR conduz |
| — | `PUL+` | laranja | **+5 V fixo** (ânodo do opto) |
| — | `DIR+` | laranja | **+5 V fixo** (ânodo do opto) |
| — | `ENA+` / `ENA-` | — | **desconectados** (driver já inicia habilitado) |
| `C`–`G` | — | — | canais livres |

- Topologia desenhada e decidida em 22/11/2026: **anodo comum**. Se a etiqueta do seu exemplar mostrar outra topologia, **pare** e atualize o esquema.
- O ULN2003A **inverte** o sinal: nível ALTO na entrada drena a saída correspondente. Os LEDs D1/D2 acendem junto com STEP e DIR, servindo de indicador visual.
- **P5:** conferir no osciloscópio qual borda em `PUL-` o driver reconhece como passo e se o sentido de `DIR` ficou invertido. Se invertido, registrar a correção em `HARDWARE-DECISIONS.md` e ajustar o firmware com o gate ainda fechado.

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

1. `GND` da SIMCAM (pino 5) **e** pino `GND` do ULN2003A — referência dos drenos `A`/`B`;
2. `GND` da fonte 5 V (junto com o `VCC` do ULN);
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
| 1 | Conferir manual/etiqueta do DM556D e o módulo ULN2003A | anodo comum, corrente do opto, jumper dos LEDs, pulso mínimo e origem do 5 V → fecha **P1** |
| 2 | Conferir 8 fios do motor com ohmímetro e ajustar DIP | paralela correta, 2,8 A → fecha **P2** |
| 3 | Conferir fonte 24 V, fusível, bitolas, star ground | dimensionamento fechado → fecha **P3** |
| 4 | Montar sinais: `In1`/`In2` ← SIMCAM, `GND` do ULN ← star ground, `VCC` ← 5 V, `A`/`B` → `PUL-`/`DIR-`, `+5 V` → `PUL+`/`DIR+`, `ENA` solto | 5 V medido antes de ligar optos |
| 5 | Montar potência: fonte → driver → motor, **motor desacoplado** | sem carga mecânica |
| 6 | Medir `PUL-`/`DIR-` com osciloscópio e conferir `+5 V` em `PUL+`/`DIR+` | afundamento pelo ULN e polaridade de anodo comum → parte de **P5** |
| 7 | Conferir borda de `STEP` e sentido de `DIR` depois da inversão do ULN | inversão registrada/corrigida em decisão; gate ainda fechado → fecha **P5** |
| 8 | Medir curso até batentes e definir limite por software | fecha **P4** |
| 9 | Observar boot/reset do driver com carga desacoplada | sem pulso falso → fecha **P5** |
| 10 | Só então acoplar ao duto/câmera e calibrar ganho/curso | registrar `VENTISEG_ENABLE_GPIO=1` em decisão |

**Enquanto os itens 1–9 não estiverem medidos e registrados:** não energizar driver/motor, não liberar os gates de compilação e não integrar captura de câmera real. Nenhuma gravação nova foi feita; esta resposta é apenas documentação dos pares.
