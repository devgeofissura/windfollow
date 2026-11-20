# Protocolo ventiseg v0.1 (bancada)

Origem do alvo: a **detecção da pessoa acontece na própria SIMCAM**, que é a
única placa do projeto. Não há câmera externa, controladora separada nem PC no
laço. O comando `T` abaixo representa o resultado dessa detecção interna
(`x` = erro de posição no quadro; `valid` = pessoa detectada), transformado
em alvo de pan pelo firmware.

**Malha fechada:** a câmera gira junto com o duto. Cada movimento do eixo
altera o quadro seguinte, então `x` é um erro relativo que precisa ser
compensado pelo controlador de pan; não é um ângulo absoluto fixo no ambiente.

Transporte: nesta entrega o firmware é **serial-only** e não abre socket. A
porta **4211** fica reservada para uma futura interface de depuração (a 4210 é
do laserseg); não é um link câmera→controladora, porque a câmera e a
controladora são a mesma placa. Endereços e autenticação `NAO CONFIRMADO`.

Formato de comando (ASCII, uma linha, sem espaços):

| Comando | Formato | Efeito |
|---|---|---|
| Seguir alvo | `TRACK,<0\|1>` | Liga/desliga o seguimento do eixo único |
| Manual | `MANUAL,<-1\|0\|1>` | Passo manual para trás/parar/frente |
| Alvo | `T,<seq>,<x>,<valid>` | `x` em `[-1000,1000]`, `valid` é `0` ou `1` |

Regras de validação (idênticas em espírito ao laserseg, sem os campos de luz/tilt):

- Linha com mais de 96 bytes é rejeitada.
- Sequência `seq` só aceita se for maior que a última aceita.
- `TRACK`/`MANUAL` com valores fora do conjunto são rejeitados.
- Datagrama inválido **não altera estado**.
- Watchdog de **1200 ms**: sem comando `T` aceito nesse prazo, o eixo para e
  `fault=Lnk`.
- `now_ms` de campo deve vir da mesma origem monotônica usada no `feed()`.
  Nunca enviar `now_ms=-1` em operação real.

Constants provisionais (reaproveitadas do laserseg, a calibrar com a mecânica):

```
kDeadBand   = 55
kTrackGain  = 0.75
kMaxTravel  = 533        # passos (provisório)
kLinkTimeoutMs = 1200
```

Telemetria de resposta:

```
ACK,<seq>,<state>,<fault>
```

- `state`: `SAFE` | `TRACK` | `MANUAL`
- `fault`: `None` | `Lnk` | `Range`
- `seq = -1` antes da primeira amostra aceita.

Boot seguro: eixo parado, `state=SAFE`, `fault=None`, sem GPIO ativo enquanto
`VENTISEG_ENABLE_GPIO=0`.

## Condições antes de habilitar GPIO (P1–P5)

1. Conferir o manual/foto do DM556D Azaneli e a lógica PUL/DIR/ENA.
2. Confirmar corrente do motor e ajuste do driver.
3. Confirmar fonte 24 V, fusível e bitolas.
4. Definir limite por software e comportamento em batente.
5. Definir estado do eixo no boot/reset/watchdog com carga desacoplada.
