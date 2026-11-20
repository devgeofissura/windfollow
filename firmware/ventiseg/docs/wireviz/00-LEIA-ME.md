# Documentação de hardware em WireViz — ventiseg

Este diretório mantém **um único projeto enxuto**, coerente com a decisão do
subprojeto: uma placa, um motor, um driver.

| Arquivo | Conteúdo |
|---|---|
| [`ventiseg-pan-24v.yaml`](ventiseg-pan-24v.yaml) | Esquema + diagrama de ligação em nível de terminal: SIMCAM → interface 3,3 V → DM556D 24 VDC → 1 motor, com referência comum |
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
- Não há **laser, luz, módulo relé, fim de curso, eixo de tilt**, nem fontes
  **12 V** ou **5 V** neste projeto. Não desenhe esses elementos por analogia
  com o `laserseg`.
- `VENTISEG_ENABLE_GPIO=0`: o firmware de bancada é serial-only. O YAML
  documenta a intenção de ligação; ele não autoriza energizar nada.

## Como renderizar

Requer WireViz 0.4.1. Nesta máquina o CLI está em
`%LOCALAPPDATA%\Programs\Python\Python314\Scripts\wireviz.exe`:

```powershell
& "$env:LOCALAPPDATA\Programs\Python\Python314\Scripts\wireviz.exe" -f g -o "$env:TEMP\opencode\wireviz-gv" "firmware\ventiseg\docs\wireviz\ventiseg-pan-24v.yaml"
```

O parser validou o arquivo sem traceback e gerou
`%TEMP%\opencode\wireviz-gv\ventiseg-pan-24v.gv`. SVG/PNG dependem do Graphviz
(`dot`) no PATH e não são necessários para a validação do YAML.

## Confirmado — pode constar como fato

Fonte primária: serigrafia/decisões do usuário em 20/11/2026.

| Item | Valor |
|---|---|
| Placa | ESP32-S3 LILYGO T-SIMCAM, 16 MB flash |
| `J_GPIO_LIVRES` (preto, 6 pinos) | `1 IO3`, `2 IO46`, `3 IO45`, `4 IO21`, `5 GND`, `6 3V3` |
| Sinais | `STEP = IO3` (PUL), `DIR = IO21` |
| Reservas | IO45 e IO46 são **strapping**: não puxar HIGH no boot (IO46) e não alterar a tensão da Flash no boot (IO45) |
| Elétrica | Lógica de **3,3 V**; nunca aplicar 5 V nos pinos de sinal; 3V3 é só para sensores |
| Driver / alimentação | DM556D (Azaneli) com **24 VDC**; GND comum em ponto único |
| Ausências | Sem laser, relé, fim de curso e tilt |

## NAO CONFIRMADO — não montar/energizar sem verificar

Tudo abaixo está marcado em `notes:` no YAML e corresponde às pendências
[P1–P5](../ESTADO.md):

| ID | Incerteza |
|---|---|
| P1 | Entradas PUL/DIR/ENA do DM556D Azaneli: comum-ânodo x comum-catodo, corrente dos optos, lógica de ENA e necessidade de conversor de nível. Mesmo modelo nominal pode ter revisão diferente do exemplar do `laserseg`. |
| P2 | Modelo, corrente e torque do motor; ajuste de corrente do driver. |
| P3 | Potência da fonte 24 V, fusível, bitolas e dissipação. |
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
- [x] Incertezas explícitas em `notes`, sem "chutar" interface elétrica.
- [x] Nenhum elemento de laser/relé/fim de curso/tilt ou fonte 12 V/5 V.
- [ ] Auditoria elétrica fechada (P1–P5) antes de qualquer `VENTISEG_ENABLE_GPIO=1`.
