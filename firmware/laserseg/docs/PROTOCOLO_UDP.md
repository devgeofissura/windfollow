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
| Direção | Câmera → controladora (`4210`); telemetria de volta definida abaixo (envio ainda não implementado) |

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

## Validação obrigatória (contrato v0.1)

O formato no fio **não mudou**. O que mudou foi o conjunto de regras que a
controladora de produção deve aplicar antes de agir. Os protótipos em
`docs/example/` usam `String.toInt()` e atualizam `lastPacket` antes do parse;
eles **não** satisfazem este contrato e não devem ser copiados como produção.

### Ordem das verificações

1. **Tamanho/codificação** — 1 a 96 bytes, ASCII puro, sem NUL.
2. **Formato** — comando exato, número exato de campos, decimais sem espaços,
   sem `+`, sem ponto decimal. Zeros à esquerda são aceitos (`007` → `7`).
3. **Faixa** — `seq` de `0` a `2147483647`; `x`/`y` de `-1000` a `1000`;
   flags `0`/`1`; `MANUAL` com eixo `pan`/`tilt` e direção `-1`/`1`.
4. **Sequência** — em amostras `T`, `seq` deve ser maior que o último aceito.
   Repetido ou regressivo é descartado. `TRACK,1` reinicia a janela, para
   que a câmera possa reiniciar seu contador sem reboot da controladora.
5. **Idade** — aplicada a **todo** comando, não só a amostras: se
   `now - rx > 1200 ms`, descartar. Exatamente `1200 ms` é aceito; o limite é
   o mesmo `LINK_TIMEOUT_MS` do watchdog.

Motivos de rejeição: `length`, `encoding`, `format`, `range`, `sequence`,
`age`. Somente datagramas **aceitos** atualizam `lastPacket` e o estado da
sessão. Um datagrama rejeitado não move eixos, não muda a luz e não altera
`last_seq`.

No boot e em qualquer watchdog é obrigatório **parar ambos os eixos** na
posição atual e desligar a luz — não basta limpar a flag de rastreamento. O
estado `axes_stopped` em `Session` torna essa ação explícita e testável.

### Especificação executável

A tabela de decisões está em `tools/protocol_ref.py` e é verificada por
**84 testes** em `tests/test_protocol.py`:

```powershell
powershell -ExecutionPolicy Bypass -File firmware\laserseg\tests\run.ps1
```

O módulo Python é a referência de comportamento; o firmware C++ de produção
deve reproduzir a mesma tabela, incluindo os limites exatos (`DEAD_BAND = 55`
inclusive, truncamento para zero em `x * MAX_TRAVEL * GAIN / 1000`).

### Implementação C++ e autoteste embarcado

O nó `controller/` espelha a referência em `parser.h`/`parser.cpp`. Os vetores
de teste **não são uma cópia manual**: `tools/gen_autotest_vectors.py` calcula
cada resultado esperado chamando `protocol_ref.py` e gera
`controller/autotest_vectors.h`. Assim, divergência entre host e placa aparece
como falha no autoteste.

```powershell
# regera os vetores apos qualquer mudanca no contrato
& "$env:LOCALAPPDATA\Programs\Python\Python314\python.exe" `
    tools\gen_autotest_vectors.py controller\autotest_vectors.h
# na placa, pela serial a 115200: digite "run" + Enter para repetir
```

O autoteste imprime `AUTOTEST PASS`/`AUTOTEST FAIL` com total, acertos e
falhas, cobrindo os 55 vetores de decisão mais asserções de sessão, watchdog,
telemetria e limites de movimento. O sketch não configura GPIO, motores, luz
nem Wi-Fi: ele apenas inicializa a serial e imprime o resultado.

### Telemetria de retorno (contrato v0.1)

Formato serial/emissor (ainda sem transporte de rede):

```
ACK,<seq>,<state>,<fault>,<light>
```

| Campo | Valores | Significado |
|---|---|---|
| `seq` | `-1` ou último `seq` aceito | `-1` antes da primeira amostra aceita |
| `state` | `SAFE`, `TRACK`, `MANUAL` | `SAFE` quando `axes_stopped` é verdadeiro |
| `fault` | `NONE`, `LENGTH`, `ENCODING`, `FORMAT`, `RANGE`, `SEQUENCE`, `AGE`, `LINK` | `LINK` = watchdog de link disparou; `AGE` = comando expirado |
| `light` | `0`/`1` | espelho do estado lógico da luz |

A função pura `formatTelemetry()` vive em `parser.cpp`; quando o envio UDP for
implementado, ele deve transmitir exatamente esta linha (sem CRLF no payload).

### Condições obrigatórias antes de acionar hardware

A auditoria de hardware aprovou a gravação deste bloco somente serial. Para a
integração com motores/LED, **todas** as condições abaixo devem ser fechadas e
revisadas novamente:

1. **Rejeição não pode deixar a máquina ativa.** Hoje um datagrama inválido,
   `seq` repetido ou expirado apenas registra `fault()`. A camada de
   acionamento deve tratar `fault() != NONE` como parada imediata de **ambos**
   os eixos e luz OFF, com precedência sobre `axesStopped()`/`light()` — ou a
   spec/testes/C++ devem mudar para forçar esse estado na rejeição.
2. **Chamar `session.poll(millis())` antes de qualquer `run()` de motor**, no
   mesmo ciclo, aplicando as três ações (parar pan, parar tilt, luz OFF).
3. **Nunca usar o `now_ms` default (-1) em campo:** chamar sempre
   `feed(buf, len, rx_ms, millis())`; o default existe apenas para os testes.
4. **Wrap-around do `millis()` (~49,7 dias):** a subtração atual em `int64_t`
   fica negativa após o rollover e o watchdog não dispara. Antes da integração,
   trocar por delta modular de 32 bits e adicionar teste de fronteira.
5. Checklist elétrico/mecânico completo em
   `.opencode/skills/hardware-safety/SKILL.md`: sensor e pinout confirmados,
   fins de curso NF, fonte separada com GND comum, driver dimensionado, LED
   difuso antes de qualquer laser e chave de emergência.
6. UDP sem autenticação: restringir à bancada e prever filtro de origem ou
   intertravamento físico antes de qualquer saída física.

## Regras para evolução

1. Validar formato, faixa numérica, sequência e idade da amostra antes de aceitar comando.
2. Rejeitar sequência repetida ou amostra expirada.
3. Nunca aceitar posição absoluta sem limites locais na controladora.
4. Manter LED/laser desligado no boot, no reset, na perda de câmera e no watchdog.
5. Se o formato mudar, atualizar `camera_node.ino`, as duas controladoras, este documento e `docs/ESTADO.md` no mesmo commit.
6. Telemetria `ACK` definida acima; ainda falta transporte de rede para a câmera.
