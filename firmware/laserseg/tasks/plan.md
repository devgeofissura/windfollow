# Plano de implementação: DM556D + NEMA 23 (V1)

Spec de origem: `firmware/laserseg/spec/SPEC-dm556d-nema23.md` (aprovada em 2026-11-19)
Status do plano: proposto — aguardando revisão humana

## Visão geral

Ligar o contrato UDP v0.1 já validado a uma camada de acionamento com dois
motores NEMA 23 23HS7628-TB6600 e dois drivers DM556D (pan/tilt), **sem**
duplicar a tabela de decisão e **sem** regredir a segurança. O firmware
serial-only atual (`controller/`) continua como baseline e referência de
autoteste até que as decisões de hardware sejam fechadas e auditadas.

## Decisões de arquitetura

1. **Nó novo, baseline intacto.** Criar `controller_dm556d/`; não editar a
   lógica de `controller/controller.ino` além do necessário para compartilhar
   o núcleo. O `.ino` principal deve ter o mesmo nome da pasta (Arduino CLI).
2. **Uma única tabela de decisão.** `parser.h`/`parser.cpp`/`autotest*` devem
   ser compilados de um único lugar (biblioteca local em `firmware/laserseg/lib/`).
   Nada de copiar `autotest_vectors.h` à mão; continua gerado por
   `tools/gen_autotest_vectors.py`.
3. **GPIO desligado por padrão.** Um define de compilação
   `LASERSEG_ENABLE_GPIO` (default `0`) separa o build serial-only do build
   com saídas físicas. Nenhuma gravação com GPIO antes do gate de auditoria.
4. **Falha para parada.** `fault() != NONE`, link > 1200 ms, fim de curso NF ou
   comando inválido ⇒ `stop()` em ambos os eixos + luz OFF, nessa ordem.
   `session.poll(millis())` sempre antes de qualquer `run()` do motor.
5. **Porta nunca fixa.** Detecção com `tools/flash.ps1 -Sketch laserseg
   -DetectOnly` antes de compilar/gravar; a placa detectada hoje (COM4,
   ESP32-S3, MAC `dc:da:0c:28:25:30`) é a T-SIMCAM de bancada, não
   necessariamente a controladora final.
6. **Credenciais fora do repo.** SSID/senha via `Preferences` ou define local
   não commitado; nada de segredo em `.ino`, docs ou plano.

## Lista de tarefas

### Fase 1 — Fundação e gates de decisão

- [ ] **Tarefa 1:** Fechar as decisões de hardware (somente documento)
  - Fechar os 7 pontos da spec: etiqueta/corrente do motor, DIP SW1–SW8,
    esquema 3,3 V → opto, identidade da placa controladora, pinos reais,
    SSID/modo de rede, chave de emergência.
  - Resultado em `docs/HARDWARE-DECISIONS.md`, referenciado por
    `docs/foco-luz-seguidor-001`.
  - **Aceite:** todos os 7 pontos respondidos ou marcados como bloqueio
    explícito; `hardware-auditor` aprovou pinagem, fontes, fins de curso e LED.
  - **Verificar:** leitura do auditor + checklist P1–P6 em `PROTOCOLO_UDP.md`.
  - **Arquivos:** `docs/HARDWARE-DECISIONS.md` (novo), `docs/foco-luz-seguidor-001`.
  - **Escopo:** S. **Depende de:** nenhuma. **Gate:** humano + hardware-auditor.

- [ ] **Tarefa 2:** Preservar e isolar o núcleo de protocolo
  - Copiar `parser.*`, `autotest.*` e `autotest_vectors.h` para
    `firmware/laserseg/lib/laserseg_core/` mantendo os originais intactos até
    confirmação; ajustar `controller/` para consumir a biblioteca.
  - **Aceite:** 84 testes host verdes; autoteste serial 76/76 na placa;
    vetores regenerados byte a byte idênticos; nenhuma lógica duplicada.
  - **Verificar:** comandos de teste, geração de vetores, compile e autoteste.
  - **Arquivos:** `lib/laserseg_core/*`, `controller/controller.ino`,
    `tools/gen_autotest_vectors.py` (se necessário).
  - **Escopo:** M. **Depende de:** nenhuma (pode correr em paralelo com T1).

**Checkpoint — Fundação**
- [ ] 84 testes green, vetores idênticos, autoteste 76/76, compile exit=0
- [ ] Decisões de hardware aprovadas por humano e auditor
- [ ] Revisão humana antes de prosseguir

### Fase 2 — Camada de movimento (ainda sem GPIO)

- [ ] **Tarefa 3:** constantes e conversão de erro em passos
  - `driver_config.h` com `STEPS_PER_REV=3200`, `MAX_TRAVEL=533`, `DEAD_BAND=55`,
    `TRACK_GAIN=0.75f`, `LINK_TIMEOUT_MS=1200`; funções puras de mapeamento e
    truncamento, sem tocar em hardware.
  - **Aceite:** constantes conferidas com a spec; funções puras testáveis;
    nenhuma escrita em GPIO.
  - **Verificar:** testes host dos limites (1200/1201 ms, wrap do `millis()`).
  - **Arquivos:** `controller_dm556d/driver_config.h`, `motion.h`, `motion.cpp`,
    `tests/test_motion.py`.
  - **Escopo:** S. **Depende de:** T1 (DIP/corrente), T2 (núcleo).

- [ ] **Tarefa 4:** sketch `controller_dm556d` serial-only
  - `.ino` com banner, comando `run` e autoteste idênticos ao baseline;
    `LASERSEG_ENABLE_GPIO=0`.
  - **Aceite:** compila `exit=0` na FQBN `laserseg`, build em `%TEMP%`;
    autoteste PASS na placa; nenhum pino configurado.
  - **Verificar:** compile + gravação + saída serial do autoteste.
  - **Arquivos:** `controller_dm556d/controller_dm556d.ino`.
  - **Escopo:** S. **Depende de:** T2, T3.

**Checkpoint — Núcleo de movimento**
- [ ] Autoteste PASS com o nó novo, sem GPIO
- [ ] `laserseg-reviewer` aprovou antes de qualquer commit
- [ ] Revisão humana antes de habilitar saídas físicas

### Fase 3 — Integração física (somente após os gates)

- [ ] **Tarefa 5:** STEP/DIR, fins de curso NF e LED sob flag
  - Habilitar `LASERSEG_ENABLE_GPIO=1` apenas com a pinagem aprovada em T1;
    `INPUT_PULLUP`, parada antes de `run()`, LED OFF no boot/fault/watchdog.
  - **Aceite:** auditoria elétrica aprovou; nenhum GPIO em `controller/`;
   光 apagada em todos os caminhos de falha.
  - **Verificar:** ensaio de bancada sem carga, fonte limitada, fio do fim de
    curso desconectado, watchdog por perda de pacote.
  - **Arquivos:** `controller_dm556d/motion.cpp`, `driver_config.h`.
  - **Escopo:** M. **Depende de:** T1, T4. **Gate:** hardware-auditor.

- [ ] **Tarefa 6:** UDP 4210 na controladora
  - `WiFiUDP`, `feed(buf, len, rx_ms, millis())`, `poll(millis())` antes dos
    motores, `last_packet_ms` atualizado somente para datagramas aceitos.
  - **Aceite:** sem `String.toInt()`; nenhum pacote inválido move eixo; sem
    credencial no código.
  - **Verificar:** testes host + bancada com datagramas bons e ruins.
  - **Arquivos:** `controller_dm556d/controller_dm556d.ino`, `network.h/cpp`.
  - **Escopo:** M. **Depende de:** T4, T5.

**Checkpoint — Integração**
- [ ] Bancada sem carga: direção, limites NF, watchdog, luz OFF observados
- [ ] Sem aquecimento ou corrente anormal
- [ ] Revisão humana antes de documentar e commitar

### Fase 4 — Fechamento

- [ ] **Tarefa 7:** documentação e commit único
  - Atualizar `README.md`, `ESTADO.md`, `PROTOCOLO_UDP.md`, `history.md` e
    marcar a spec como `implemented` quando aplicável.
  - **Aceite:** `laserseg-reviewer` aprovou; `tools/save.ps1` com DEVLOG e push.
  - **Verificar:** `git status` limpo após o push.
  - **Arquivos:** docs listadas + `DEVLOG.md`.
  - **Escopo:** M. **Depende de:** T1–T6.
  - **Atenção:** suas alterações não commitadas em `docs/example/` →
    `example01/`/`example02/` são suas; `tools/save.ps1` usa `git add -A` e as
    incluirá. Decida antes se quer commitá-las separadamente ou aguardar.

## Riscos e mitigação

| Risco | Impacto | Mitigação |
|---|---|---|
| GPIO 3,3 V ligado direto nos optos do DM556D | Alto (queima de pino/driver) | Interface apropriada obrigatória; gate T1 + auditoria |
| Corrente do motor acima do ajuste do driver | Alto (aquecimento) | Confirmar etiqueta e DIP antes de energizar; fonte limitada no ensaio |
| Duplicação da tabela de decisão | Médio (divergência de segurança) | Biblioteca única `laserseg_core`, vetores gerados |
| Placa de bancada confundida com controladora final | Médio | Detecção sempre antes de gravar; T1 define a placa real |
| `git add -A` incluir alterações do usuário | Médio | Não commitar até decisão explícita sobre `example01/02` |
| Watchdog / wrap do `millis()` mal tratado | Alto (eixo solto) | Testes de 1200/1201 ms e rollover; `poll()` antes de `run()` |

## Perguntas em aberto

As mesmas 7 da spec (etiqueta/corrente, DIP, interface elétrica, placa
controladora, pinos, rede, intertravamento). Elas **bloqueiam T5 e T6**, mas
não impedem T2–T4, que são serial-only e seguros.
