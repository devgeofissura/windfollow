# Simulador Local do ventiseg (V0.1)

Página local para exercitar o contrato v0.1 do ventiseg **sem placa**: FSM
`SAFE/TRACK/MANUAL`, watchdog, telemetria `ACK` e pinagem somente leitura.
Não há Wi-Fi, luz, laser, relé, tilt nem fim de curso neste projeto.

> **Isto é uma simulação.** Nenhum GPIO, motor ou iluminador real é acionado.
> Os comandos ficam neste navegador. O firmware de bancada mantém
> `VENTISEG_ENABLE_GPIO = 0`.

## Como abrir

Duplo clique em `index.html` — funciona em `file://`, sem servidor, sem rede,
sem build e sem bibliotecas externas. Não é preciso instalar nada.

## O que simula

- **Painel:** eixo único (pan do fluxo de ar) com posição comandada em passos,
  estado da controladora, botões de rastreamento e manual, injeção de qualquer
  linha do protocolo, perda de link, falha `Range` e log `TX`/`RX`/`EV`.
  Em `TRACK`, amostras `T` sintéticas alimentam o eixo continuamente.
  Na bancada real, a SIMCAM detecta a pessoa e gera essas amostras; câmera e
  duto giram juntos, então a visão é malha fechada (o erro detectado precisa
  compensar o próprio movimento). Aqui não há imagem nem detecção real.
- **Saída STEP/DIR:** parâmetros configuráveis (velocidade, aceleração,
  micropasso), gate pela web com modal de confirmação e telemetria lógica
  (alvo, posição, pulsos, DIR/STEP). Algoritmo não bloqueante: no máximo um
  pulso por `update()`, rampa limitada por aceleração e distância de frenagem.
  A liberação é **somente na simulação**; o firmware exige gates de compilação
  e auditoria elétrica antes de qualquer saída física. O formulário de ensaio
  emite N pulsos exatos com prioridade sobre o alvo da FSM — ainda somente
  lógico, sem escrita de pino.
- **Protocolo:** constantes e formatos do contrato v0.1, somente leitura.
- **Pinagem:** conector `J_GPIO_LIVRES` (STEP=IO3, DIR=IO21, IO45/46
  reservados por strapping, GND, 3V3), lógica 3,3 V e driver DM556D Azaneli
  @ 24 VDC. **Somente leitura:** não altera estado nem aciona hardware.

## Contrato espelhado

| Comando | Formato |
|---|---|
| Seguir alvo | `TRACK,<0\|1>` |
| Manual | `MANUAL,<-1\|0\|1>` |
| Alvo | `T,<seq>,<x>,<valid>` (`x` em `[-1000,1000]`) |
| Telemetria | `ACK,<seq>,<state>,<fault>` |

Plano de bancada da saída (fora do parser validado): `CFG,?`,
`CFG,SPEED,<n>`, `CFG,ACCEL,<n>`, `CFG,MICRO,<8|16>`, `CFG,GATE,0`,
`CFG,GATE,1,CONFIRM` e `CFG,PULSE,<1..1000>,<+|-1>`. Respostas `CFG,...,OK` ou
`REASON,...`; não geram `ACK` e não mudam estado/fault/sequência. `CFG,?`
também informa `test=<pulsos restantes>`.

Constantes: `kMaxDatagramBytes=96`, `kDeadBand=55`, `kManualStep=80`,
`MAX_TRAVEL=533`, `TRACK_GAIN=0.75`, `kLinkTimeoutMs=1200`.
`state` ∈ `SAFE|TRACK|MANUAL`; `fault` ∈ `None|Lnk|Range`; `seq=-1` antes da
primeira amostra aceita. Boot `SAFE`, eixo parado. Linha > 96 bytes, formato
inválido, `x` fora da faixa ou `seq` não crescente são rejeitados **sem mudar
estado**. Sem comando aceito em 1200 ms: watchdog força `SAFE`, eixo parado e
`fault=Lnk`. O `now_ms` do núcleo vem de uma origem monotônica injetada
(`performance.now()` na UI), a mesma usada no `feed()` e no watchdog.

## Regras de segurança

- `VENTISEG_ENABLE_GPIO = 0` e `VENTISEG_HW_AUDIT_OK = 0`: nenhuma escrita em
  GPIO, nenhum motor energizado. Mesmo o gate da web não altera isso.
- A verificação foi **lógica** (firmware e testes host): parser, autoteste,
  watchdog, camada STEP/DIR e ensaio de pulsos em Python × JS × C++. Não houve
  osciloscópio em `PUL+`/`DIR+`, driver alimentado, teste de boot/reset nem
  carga desacoplada. O ensaio da UI não mede pulso elétrico algum.
- Não há aquisição de câmera, rede, `localStorage` nem dependências externas.
- As constantes exibidas espelham `docs/PROTOCOLO.md`; testes host barram
  divergência e exercitam a FSM em Node com tempo determinístico.

## Validação

```powershell
node --check firmware/ventiseg/simulator/js/app.js
python -m unittest discover -s firmware/ventiseg/tests -p 'test_simulator.py' -v
```

## Limitações

- Não integra com firmware real, serial, UDP nem gravação na placa.
- O gerador de alvo usa valores aleatórios apenas na UI; a FSM testada é pura.
- Tipos de entrada do DM556D, corrente do motor e curso útil seguem
  `NAO CONFIRMADO` em `docs/HARDWARE-DECISIONS.md`.
