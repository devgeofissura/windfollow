# Documentação de hardware em WireViz — ventiseg

Este diretório mantém **um único projeto enxuto**, coerente com a decisão do
subprojeto: uma placa, um motor, um driver.

| Arquivo | Conteúdo |
|---|---|
| [`ventiseg-pan-24v-diagrama.yaml`](ventiseg-pan-24v-diagrama.yaml) | **Fonte técnica**: 7 conectores, 24 cabos unifilares, 26 conexões, `loops` do GND estrela e os pinos `ENA+`/`ENA-` isolados. Não alterar sem revisão elétrica. |
| [`ventiseg-pan-24v-diagrama.gv`](ventiseg-pan-24v-diagrama.gv) | **Layout organizado** gerado pelo SCAD Studio: SIMCAM à esquerda, conversor ao centro, DM556D/motor à direita, fontes abaixo, GND único, legenda e quadro de avisos em nós separados. |
| [`ventiseg-pan-24v-diagrama.svg`](ventiseg-pan-24v-diagrama.svg), [`.png`](ventiseg-pan-24v-diagrama.png), [`.png` 200 dpi](ventiseg-pan-24v-diagrama-hires.png) | **Renderizações Graphviz** do `.gv` organizado. O PNG de alta resolução mede 9655 × 2532 px. |
| [`ventiseg-pan-24v.yaml`](ventiseg-pan-24v.yaml) | Esquema original em nível de terminal: SIMCAM → conversor 3,3 V→5 V → DM556D 24–36 V → motor KTC-HT23-400 em bipolar paralela, com referência comum |
| [`../LIGACOES.md`](../LIGACOES.md) | Guia de execução: pino a pino, polaridade do opto, fases do motor, alimentação e ordem de ensaio |

## Por que é enxuto

- **Uma** ESP32-S3 LILYGO T-SIMCAM faz a visão **e** comanda o driver — não
  existe uma controladora separada (ao contrário do `laserseg`). Essa é uma
  decisão registrada em [`../HARDWARE-DECISIONS.md`](../HARDWARE-DECISIONS.md).
- **A SIMCAM detecta a pessoa e comanda o pan**: visão e controle na mesma
  placa, sem câmera externa nem controladora separada.
- **Câmera e duto giram juntos** (malha fechada): placa, câmera e saída de ar
  são um único conjunto móvel acionado pelo **único** motor de passo via
  DM556D @ 24 VDC. O suporte e a folga do cabo nesse conjunto são
  `NAO CONFIRMADO`; não afetam esta ligação STEP/DIR.
- Não há **laser, luz, módulo relé, fim de curso, eixo de tilt** nem fonte
  **12 V** neste projeto. Não desenhe esses elementos por analogia com o
  `laserseg`. Existe **uma fonte de 5 V**, apenas para o lado HV do conversor
  de nível; sua origem está em `NAO CONFIRMADO` (P1).
- `VENTISEG_ENABLE_GPIO=0`: o firmware de bancada é serial-only. O YAML
  documenta a intenção de ligação; ele não autoriza energizar nada.

## Como renderizar

No **SCAD Studio**, abra a aba WireViz, solte
`ventiseg-pan-24v-diagrama.yaml`, mantenha **LAYOUT = Organizado** e clique
em **Gerar diagrama**. O aplicativo converte o YAML em DOT e renderiza o SVG
com Graphviz WASM, localmente. Use os botões **DOT**, **SVG** e **PNG**
(RESOLUÇÃO PNG = 4×) para exportar.

Para regenerar os PNGs entregues a partir do `.gv` já versionado (Graphviz
instalado em `C:\Program Files\Graphviz\bin`):

```powershell
$env:PATH = "C:\Program Files\Graphviz\bin;$env:PATH"
$base = "firmware\ventiseg\docs\wireviz\ventiseg-pan-24v-diagrama"
dot -Tpng -Gdpi=120 "$base.gv" -o "$base.png"
dot -Tpng -Gdpi=200 "$base.gv" -o "$base-hires.png"
```

Não rode `wireviz -f gsp -o firmware\ventiseg\docs\wireviz ...`: isso
substituiria o DOT organizado pelo desenho espalhado no estilo WireViz.
Para validar o YAML com o WireViz original, escreva apenas em `%TEMP%`:

```powershell
& "$env:LOCALAPPDATA\Programs\Python\Python314\Scripts\wireviz.exe" -f g -o "$env:TEMP\opencode\wireviz-gv" "firmware\ventiseg\docs\wireviz\ventiseg-pan-24v.yaml"
```

Validação do modo organizado (no repositório `E:\dev\scadstudio`):

```powershell
node scripts\check-wireviz-schematic.mjs "E:\dev\windfollow\firmware\ventiseg\docs\wireviz\ventiseg-pan-24v-diagrama.yaml"
```

## Confirmado — pode constar como fato

Fonte primária: serigrafia/decisões do usuário em 20/11/2026.

| Item | Valor |
|---|---|
| Placa | ESP32-S3 LILYGO T-SIMCAM, 16 MB flash |
| `J_GPIO_LIVRES` (preto, 6 pinos) | `1 IO3`, `2 IO46`, `3 IO45`, `4 IO21`, `5 GND`, `6 3V3` |
| Sinais | `STEP = IO3` (PUL), `DIR = IO21` |
| Reservas | IO45 e IO46 são **strapping**: não puxar HIGH no boot (IO46) e não alterar a tensão da Flash no boot (IO45) |
| Elétrica | Lógica de **3,3 V**; nunca aplicar 5 V nos pinos; conversor 3,3→5 V obrigatório |
| Interface | **Catodo comum**: GND em `PUL-`/`DIR-`, 5 V em `PUL+`/`DIR+`; `ENA` desconectado |
| Driver / alimentação | DM556D (24–50 VDC, 2,1–5,6 A) com **24–36 VDC / mín. 4 A**; GND comum em ponto único |
| Motor | **KTC-HT23-400**, 8 fios, bipolar paralela 2,8 A / 1,9 Nm |
| Ausências | Sem laser, relé, fim de curso, tilt e fonte 12 V |

## NAO CONFIRMADO — não montar/energizar sem verificar

Tudo abaixo está marcado em `notes:` no YAML e corresponde às pendências
[P1–P5](../ESTADO.md):

| ID | Incerteza |
|---|---|
| P1 | Corrente dos optos, pulso mínimo e origem do 5 V do conversor. Catodo comum, conversor e `ENA` desconectado já foram confirmados; mesmo modelo nominal pode ter revisão diferente do exemplar do `laserseg`. |
| P2 | Posições exatas de SW5–SW8 (alvo 1/8 passo = 1600 pulsos/volta) e curso/batentes. Modelo, fios e corrente do motor já confirmados. |
| P3 | Potência exata da fonte 24–36 V, fusível, bitolas e dissipação. |
| P4 | Acoplamento motor↔ventilador, curso útil em passos e batentes mecânicos; sem homing físico. |
| P5 | Pulso falso no boot/reset do driver e comportamento com carga desacoplada. |

Também **NAO CONFIRMADO**: revisão física da T-SIMCAM (V1.3/V1.4) e o sensor
de câmera — irrelevantes para STEP/DIR, mas registrados para não virarem fato.

## Divergência registrada (não resolvida por suposição)

A tabela oficial da SIMCAM e a skill `espressif-lilygo` listam IO3 como ADC de
bateria e IO21 como status do modem; a serigrafia informada pelo usuário diz
que são GPIOs livres. Para o ventiseg prevalece a serigrafia (fonte primária
desta bancada), com IO45/IO46 reservados. A divergência continua documentada
em [`../HARDWARE-DECISIONS.md`](../HARDWARE-DECISIONS.md).

## Estado

- [x] YAML enxuto escrito e validado pelo parser WireViz 0.4.1.
- [x] Layout organizado em DOT/SVG/PNG com 7 blocos, legenda de cores e
      quadro de avisos separado. Auditoria: 24 arestas, 2 terminais isolados
      (`ENA+`/`ENA-`) e 3 `loops` do GND estrela — mesmas conexões do YAML.
- [x] Incertezas explícitas em `notes`, sem "chutar" interface elétrica.
- [x] Nenhum elemento de laser/relé/fim de curso/tilt ou fonte 12 V.
      A fonte 5 V existe somente para o lado HV do conversor.
- [ ] Auditoria elétrica fechada (P1–P5) antes de qualquer `VENTISEG_ENABLE_GPIO=1`.
