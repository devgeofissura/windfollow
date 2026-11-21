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

Regras de validação (mesma ordem em Python, C++ e no simulador JS; sem os
campos de luz/tilt do laserseg):

1. Tamanho/codificação: linha vazia ou com mais de 96 bytes → `length`;
   byte não-ASCII ou NUL → `encoding`.
2. Formato: decimal estrito com sinal `-` opcional; zeros à esquerda são
   aceitos (`TRACK,01`, `MANUAL,01`, `T,007,-01,01`).
3. Faixa: `TRACK` só `0/1`, `MANUAL` só `-1/0/1`, `x` em `[-1000,1000]`,
   `valid` só `0/1` e `seq` em `[0, 2147483647]` (`INT32_MAX`). Fora disso →
   `range`.
4. Sequência (somente em `T`): `seq` deve ser maior que a última aceita →
   senão `sequence`.
5. Idade (qualquer comando, por último): `age = now_ms - rx_ms` deve estar em
   `[0, 1200]` ms → senão `age`.

Datagrama rejeitado **não altera** modo, eixo, sequência nem relógio do link;
uma rejeição por `range` liga `fault=Range` como diagnóstico. O motivo é
observável na serial como `REASON,<reason>` (`length`, `encoding`, `format`,
`range`, `sequence`, `age`).

Watchdog de **1200 ms**: sem comando `T` aceito nesse prazo, o eixo para e
`fault=Lnk`. O relógio do link é `rx_ms` (recebimento), não o instante da
consulta `now_ms`. `now_ms` deve vir da mesma origem monotônica usada no
`feed()`. Nunca enviar `now_ms=-1` em operação real.

### Comportamento seguro depois de `fault=Lnk`

1. O watchdog força `state=SAFE`, eixo parado, `fault=Lnk` e link perdido.
2. Enquanto o link estiver perdido, `TRACK` e `MANUAL` **continuam sendo
   comandos válidos no parser, mas são inibidos pela sessão**: não mudam modo,
   não movem o eixo, não renovam o watchdog e a telemetria permanece
   `ACK,<seq>,SAFE,Lnk`. Assim nenhum comando de movimento pode rearmar o eixo
   sem uma amostra de visão fresca.
3. `Lnk` é dominante sobre `Range`: uma rejeição por faixa posterior não apaga
   a perda de link.
4. Somente um `T` aceito (idade válida e `seq` maior) restaura o link: limpa
   `fault`, renova o watchdog e **mantém o eixo em SAFE**. Para voltar a
   comandar movimento é necessário um novo `TRACK` ou `MANUAL` depois do `T`.
5. A camada de saída STEP/DIR permanece desabilitada por padrão; nenhuma
   transição acima gera pulso enquanto `VENTISEG_ENABLE_GPIO=0`.

Esse comportamento é verificado por `tests/test_safe_state.py`, que executa o
mesmo roteiro em Python, JavaScript e C++ e exige traces idênticos.

Constants provisionais (reaproveitadas do laserseg, a calibrar com a mecânica):

```
kDeadBand   = 55
kTrackGain  = 0.75
kMaxTravel  = 533        # passos (provisório)
kLinkTimeoutMs = 1200
kXLimit     = 1000
kInt32Max   = 2147483647
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
