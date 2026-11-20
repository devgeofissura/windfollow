# ventiseg — pan de fluxo de ar com ESP32-S3 SIMCAM

Subprojeto irmão do `laserseg`, enxuto por decisão: **uma** placa, **um**
motor, **um** driver, **sem** laser/luz, **sem** relé e **sem** fim de curso.

## Hardware confirmado

| Item | Estado |
|---|---|
| ESP32-S3 LILYGO T-SIMCAM, 16 MB flash | Confirmado na bancada (MAC `dc:da:0c:28:25:30`; porta muda, sempre redetectar) |
| Conector `J_GPIO_LIVRES` 6 pinos | Serigrafia transcrita pelo usuário |
| STEP = IO3, DIR = IO21 | Serigrafia; inversão posterior tratada como typo |
| Driver **DM556D** (24–50 VDC, 2,1–5,6 A) | 24–36 VDC / mín. 4 A |
| Motor **KTC-HT23-400**, bipolar paralela 2,8 A / 1,9 Nm | Fios e DIP em `docs/LIGACOES.md` |
| Sinal 3,3 V → conversor → 5 V, catodo comum | `ENA` desconectado; corrente do opto `NAO CONFIRMADO` |

## Arquitetura

```
   pessoa no quadro
          ^                         malha fechada (câmera gira com o duto)
          |                                       |
 [ SIMCAM: detecção + erro ] --STEP(IO3)/DIR(IO21)--> conversor 3,3→5 V --> DM556D 24-36 V --> motor --> duto + câmera
          |                                                                         |
          +--------------------------- telemetria ACK na serial --------------------+
```

- **A câmera detecta a pessoa e move o pan**: não existe placa controladora
  externa nem visão em outro computador. A detecção roda na própria SIMCAM e
  seu resultado alimenta o eixo único do firmware.
- **Câmera e duto giram juntos**: o quadro se move com o pan, então a visão é
  malha fechada — o erro detectado precisa ser compensado pelo próprio
  movimento, mantendo a pessoa centralizada e o fluxo apontado a ela.
- Sem Wi-Fi nesta primeira entrega: a detecção e o comando de pan são internos;
  o firmware de bancada é serial-only.
- Sem GPIO ativo: `VENTISEG_ENABLE_GPIO=0`.
- Limites de curso são **por software** (contagem de passos); não existe
  homing físico confirmado.

## Árvore

```
firmware/ventiseg/
├── AGENTS.md                 # regras locais e pinagem
├── README.md                 # este arquivo
├── docs/
│   ├── ESTADO.md             # status, pendências P1-P5
│   ├── HARDWARE-DECISIONS.md # fatos x NAO CONFIRMADO
│   ├── LIGACOES.md           # guia de execução: terminal a terminal
│   ├── PROTOCOLO.md          # contrato v0.1
│   └── wireviz/              # esquema enxuto
├── spec/                     # especificações aprovadas
├── tasks/                    # planos e TDD
├── tools/                    # referência executável do protocolo
├── controller/               # sketch + parser + autoteste
├── tests/                    # suíte host (Python)
└── simulator/                # bancada web local
```

## Comandos

```text
powershell -ExecutionPolicy Bypass -File firmware/ventiseg/tests/run.ps1
powershell -ExecutionPolicy Bypass -File tools/flash.ps1 -Sketch ventiseg -DetectOnly
```

A aba do simulador abre com `file://` em `firmware/ventiseg/simulator/index.html`.
