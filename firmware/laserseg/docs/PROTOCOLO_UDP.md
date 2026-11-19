# Protocolo UDP do laserseg

Documento de referência para o enlace entre o nó de visão (T-SIMCAM) e a controladora. O formato abaixo é o que os protótipos em `docs/example/` implementam; qualquer mudança deve ser replicada nos dois sketches e registrada no `DEVLOG.md`.

## Transporte

| Parâmetro | Valor |
|---|---|
| Transporte | UDP |
| Porta de escuta da controladora | `4210` |
| Endereço no modo AP | Controladora em `192.168.4.2`, câmera em `192.168.4.1` |
| Autenticação | Nenhuma no protótipo |
| Rede | Somente rede local confiável; não expor o UDP |
| Direção | Câmera → controladora; telemetria de volta ainda não definida |

> O protocolo não autentica mensagens. Um dispositivo na mesma rede pode comandar motores e luz. Não usar fora de bancada sem autenticação, filtragem de origem e limites locais na controladora.

## Mensagens enviadas pela câmera

### `TRACK,<0|1>`

Habilita ou desabilita o seguimento na controladora.

- `TRACK,1` — seguir posições recebidas.
- `TRACK,0` — parar e manter eixos na posição atual.

### `LIGHT,<0|1>`

Liga ou desliga a saída do foco.

- `LIGHT,1` — saída HIGH no pino do MOSFET/driver.
- `LIGHT,0` — saída LOW.

### `MANUAL,<axis>,<dir>`

Movimentação manual por passo.

- `<axis>`: `pan` ou `tilt`.
- `<dir>`: `-1` ou `1`.
- Cada comando desloca o alvo em 80 passos, limitado a `MAX_TRAVEL`.

### `T,<seq>,<x>,<y>,<valid>`

Observação de rastreamento.

| Campo | Tipo | Significado |
|---|---|---|
| `seq` | inteiro | Contador crescente gerado pela câmera |
| `x` | inteiro | Erro horizontal normalizado: `-1000` (borda esquerda) a `1000` (borda direita) |
| `y` | inteiro | Erro vertical normalizado: `-1000` (topo) a `1000` (base) |
| `valid` | `0`/`1` | `1` quando houve mudança de pixels suficiente na janela de análise |

Origem: centro da imagem. Valores positivos indicam que o alvo está à direita / abaixo do centro nos protótipos. confirme o sentido com ensaio antes de confiar no movimento.

A câmera envia `T,...` apenas quando `trackingEnabled` é verdadeiro. A detecção é por **diferença entre quadros**: não reconhece pessoa, animal ou objeto específico e reage a mudanças de iluminação.

## Comportamento da controladora

- Atualiza `lastPacket` a cada datagrama recebido e aceito pelo parser.
- `TRACK,0`, `valid=0` ou perda de link paravam os eixos na posição atual.
- `LINK_TIMEOUT_MS = 1200`: sem pacote nesse intervalo, parar os eixos e desligar a luz.
- `DEAD_BAND = 55`: erros dentro da zona morta não geram novo alvo.
- `TRACK_GAIN`: `0.75` no protótipo DRV8834, `0.55` no ULN2003.
- `MAX_TRAVEL = STEPS_PER_REV / 6`: limite lógico inicial de ±60°.
- Fins de curso NF com `INPUT_PULLUP`: entrada HIGH bloqueia o movimento naquele sentido. Fio rompido também bloqueia.
- A controladora **não** faz homing automático; os eixos devem começar centralizados manualmente.

## Regras para evolução

1. Validar formato, faixa numérica, sequência e idade da amostra antes de aceitar comando.
2. Rejeitar sequência repetida ou amostra expirada.
3. Nunca aceitar posição absoluta sem limites locais na controladora.
4. Manter LED/laser desligado no boot, no reset, na perda de câmera e no watchdog.
5. Se o formato mudar, atualizar `camera_node.ino`, as duas controladoras, este documento e `docs/ESTADO.md` no mesmo commit.
6. Preveracknowledgement/telemetria (`ack_seq`, `state`, `fault`) antes de operação supervisionada.
