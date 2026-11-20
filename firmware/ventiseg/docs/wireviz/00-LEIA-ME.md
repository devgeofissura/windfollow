# Documentação de hardware em WireViz — ventiseg

Este diretório mantém **um único projeto enxuto**, coerente com a decisão do
subprojeto: uma placa, um motor, um driver.

| Arquivo | Conteúdo |
|---|---|
| [`ventiseg-pan-24v-diagrama.yaml`](ventiseg-pan-24v-diagrama.yaml) | **Desenho técnico colorido**: 7 blocos retangulares + banner de avisos, ordem física Fonte 24 V → DM556D → Motor e SIMCAM → Conversor → DM556D. Entregue em `.svg`, `.png` e `.png` 200 dpi |
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

Requer WireViz 0.4.1 e Graphviz (`dot`, instalado em
`C:\Program Files\Graphviz\bin`). Comando **exato**, na raiz do repositório:

```powershell
$env:PATH = "C:\Program Files\Graphviz\bin;$env:PATH"
wireviz -f gsp -o firmware\ventiseg\docs\wireviz firmware\ventiseg\docs\wireviz\ventiseg-pan-24v-diagrama.yaml
```

Gera `.gv`, `.svg` e `.png` na própria pasta. Para o PNG em alta resolução
(200 dpi), a partir do `.gv`:

```powershell
& "C:\Program Files\Graphviz\bin\dot.exe" -Tpng -Gdpi=200 `
  "firmware\ventiseg\docs\wireviz\ventiseg-pan-24v-diagrama.gv" `
  -o "firmware\ventiseg\docs\wireviz\ventiseg-pan-24v-diagrama-hires.png"
```

Para apenas validar o YAML original sem Graphviz:

```powershell
& "$env:LOCALAPPDATA\Programs\Python\Python314\Scripts\wireviz.exe" -f g -o "$env:TEMP\opencode\wireviz-gv" "firmware\ventiseg\docs\wireviz\ventiseg-pan-24v.yaml"
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
- [x] Desenho colorido com 7 blocos, legenda de cores e banner de avisos
      gerado em SVG, PNG e PNG 200 dpi (32 nós, sem sobreposição).
- [x] Incertezas explícitas em `notes`, sem "chutar" interface elétrica.
- [x] Nenhum elemento de laser/relé/fim de curso/tilt ou fonte 12 V.
      A fonte 5 V existe somente para o lado HV do conversor.
- [ ] Auditoria elétrica fechada (P1–P5) antes de qualquer `VENTISEG_ENABLE_GPIO=1`.
