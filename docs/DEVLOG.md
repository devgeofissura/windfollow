# Diário de desenvolvimento (DEVLOG)

Registro cronológico do que foi feito em cada commit, com as dificuldades
encontradas e como foram resolvidas. Cada entrada é inserida automaticamente no
topo da seção **Histórico** por `tools/save.ps1`, que commita e faz push.

Formato de cada entrada: data/hora, mensagem do commit, o que mudou, dificuldades
e soluções, arquivos tocados.

---

## Histórico

### 2026-11-21 10:46 - fix(ventiseg): alinha simulador a faixa seq e idade

**Area:** ventiseg

**Feito:**
- teto de seq INT32_MAX e zeros a esquerda aceitos
- idade rx_ms/now_ms aplicada a qualquer comando
- rejeicao por faixa passa a ligar fault=Range
- PROTOCOLO.md ESTADO.md e painel do simulador atualizados

**Dificuldades e solucoes:**
- ageRejected retornava false ao rejeitar -> invertido para true
- harness comparava o estado inteiro -> passou a comparar o nucleo sem fault
- 139 testes host verdes apos a correcao

**Arquivos:**
- `firmware/ventiseg/docs/ESTADO.md`
- `firmware/ventiseg/docs/PROTOCOLO.md`
- `firmware/ventiseg/simulator/index.html`
- `firmware/ventiseg/simulator/js/app.js`
- `firmware/ventiseg/tests/test_simulator.py`

### 2026-11-21 10:27 - docs: versiona plano Wi-Fi laserseg e archive wireviz ventiseg

**Area:** laserseg, ventiseg

**Feito:**
- plano e todo de provisionamento Wi-Fi V1 do laserseg
- arquivo wireviz.zip do ventiseg com diagrama pan 24V para historico

**Dificuldades e solucoes:**
- save.ps1 usa git add -A e o plano laserseg pede commit seletivo -> commit unico aprovado pelo usuario
- wireviz.zip repete arquivos ja versionados em docs/wireviz -> confirmado como artefato a versionar

**Arquivos:**
- `firmware/laserseg/tasks/plan-wifi-provisioning.md`
- `firmware/laserseg/tasks/todo-wifi-provisioning.md`
- `firmware/ventiseg/docs/wireviz.zip`

### 2026-11-20 17:00 - docs(ventiseg): publica diagrama organizado no SCAD Studio

**Area:** ventiseg

**Feito:**
- adiciona modo organizado no SCAD Studio com DOT, SVG e PNG
- substitui diagrama espalhado do ventiseg por layout compacto com fontes abaixo
- atualiza guia de renderizacao e dimensoes do PNG

**Dificuldades e solucoes:**
- modo classico contava 25 linhas de cor em vez de 15 -> colorRows agora emite 3 linhas para fio simples e 5 para bicolor
- fontes subiam acima dos blocos -> invis edges com constraint=false foram trocados por constraint padrao e portas ajustadas

**Arquivos:**
- `firmware/ventiseg/docs/wireviz/00-LEIA-ME.md`
- `firmware/ventiseg/docs/wireviz/ventiseg-pan-24v-diagrama-hires.png`
- `firmware/ventiseg/docs/wireviz/ventiseg-pan-24v-diagrama.gv`
- `firmware/ventiseg/docs/wireviz/ventiseg-pan-24v-diagrama.png`
- `firmware/ventiseg/docs/wireviz/ventiseg-pan-24v-diagrama.svg`

### 2026-11-20 16:28 - docs(ventiseg): diagrama eletrico colorido com blocos e avisos

**Area:** ventiseg

**Feito:**
- novo YAML WireViz 0.4.1 com 7 blocos retangulares, cabos com legenda de cores e todos os terminais
- SVG, PNG e PNG 200 dpi gerados com Graphviz 16.1.1 e comando exato documentado
- indice wireviz e README atualizados com os novos artefatos

**Dificuldades e solucoes:**
- Graphviz ausente -> instalado Graphviz 16.1.1 e PATH ajustado
- bloco de avisos descartado por nao estar em connections -> banner injetado via tweak.append
- caracteres > em rotulos HTML quebraram o dot -> substituidos por entidades HTML no YAML
- porta/flash nao tocados porque a tarefa e apenas documental

**Arquivos:**
- `firmware/ventiseg/README.md`
- `firmware/ventiseg/docs/wireviz/00-LEIA-ME.md`
- `firmware/ventiseg/docs/wireviz/ventiseg-pan-24v-diagrama-hires.png`
- `firmware/ventiseg/docs/wireviz/ventiseg-pan-24v-diagrama.gv`
- `firmware/ventiseg/docs/wireviz/ventiseg-pan-24v-diagrama.png`
- `firmware/ventiseg/docs/wireviz/ventiseg-pan-24v-diagrama.svg`
- `firmware/ventiseg/docs/wireviz/ventiseg-pan-24v-diagrama.yaml`

### 2026-11-20 15:49 - docs(ventiseg): catodo comum, conversor 5V e motor KTC-HT23-400 no esquema

**Area:** ventiseg

**Feito:**
- esquema WireViz reescrito para catodo comum com conversor obrigatorio 3,3V para 5V e ENA desconectado
- motor KTC-HT23-400 em bipolar paralela com fios A+/A-/B+/B-, DIP SW1-SW4 e alvo de 1600 pulsos por volta documentados
- guia LIGACOES, HARDWARE-DECISIONS, ESTADO, README e AGENTS alinhados com a nova interface e o novo motor
- pinagem IO3=STEP e IO21=DIR preservada apos conflito resolvido como typo pelo usuario
- 135 testes host verdes e WireViz 0.4.1 validado sem traceback

**Dificuldades e solucoes:**
- conflito IO3/IO21 no texto do usuario -> perguntei e o usuario confirmou que a serigrafia prevalece
- WireViz rejeitou cabo de 2 condutores em um unico borne -> dividi em 8 condutores individuais e revalidei
- topologia do opto divergia do YAML commitado -> usuario escolheu catodo comum e o esquema foi reescrito

**Arquivos:**
- `firmware/ventiseg/AGENTS.md`
- `firmware/ventiseg/README.md`
- `firmware/ventiseg/docs/ESTADO.md`
- `firmware/ventiseg/docs/HARDWARE-DECISIONS.md`
- `firmware/ventiseg/docs/LIGACOES.md`
- `firmware/ventiseg/docs/wireviz/00-LEIA-ME.md`
- `firmware/ventiseg/docs/wireviz/ventiseg-pan-24v.yaml`

### 2026-11-20 15:40 - feat(ventiseg): base serial-only, simulador e ligacoes DM556D 24V

**Area:** ventiseg, raiz

**Feito:**
- novo subprojeto ventiseg com perfil S3 no flash.ps1 e tabelas da raiz
- nucleo TDD com protocol_ref, parser C++, 52 vetores e 135 testes host verdes
- simulador local file:// com FSM deterministica e watchdog alinhado ao contrato
- esquema WireViz em nivel de terminal e guia LIGACOES com interface 3,3V, fases A/B e 24V
- visao na SIMCAM e camera acoplada ao duto registradas como decisoes confirmadas

**Dificuldades e solucoes:**
- perfil ventiseg rejeitado pelo ValidateSet -> adicionado ao ValidateSet e a tabela de perfis com FQBN S3
- simulador renovava watchdog com TRACK/MANUAL -> corrigido para so T aceito e testes reforcados
- topologia do opto DM556D nao confirmada -> variante comum-anodo desenhada e alternativa documentada sem assumir como fato

**Arquivos:**
- `AGENTS.md`
- `README.md`
- `tools/flash.ps1`
- `firmware/ventiseg/`

### 2026-11-20 14:46 - feat(laserseg): aba Pinout e hardware WireViz do laser

**Area:** laserseg, raiz

**Feito:**
- 3 YAMLs WireViz versionados e validados: controladora DM556D com 27 conectores/34 cabos, pinout da controladora e pinout primario da S3_CAM_SIM
- aba Pinout somente leitura no simulador com 9 testes novos, sem tocar na FSM SAFE/TRACK/MANUAL nem em js/app.js
- laser via rele serial RoboCore documentado com conversor 3,3 a 5 V, rail 5 V, star ground, fontes separadas e contato NA + emergencia + watchdog
- mapas antigos LED_EN/GPIO12 e mapas historicos de GPIO marcados como SUPERADO sem apagar o registro, divergencias do pinout preservadas
- 110 testes host verdes, node --check e git diff --check limpos

**Dificuldades e solucoes:**
- pinout do usuario divergia da tabela oficial e do issue 33 -> adotei o pinout como fonte primaria e registrei as divergencias sem suposicao
- parser WireViz rejeitou conexao direta conector a conector e arquivo aberto como TextIOWrapper -> usei wireviz.wireviz.parse com cabo intermediario
- PowerShell 5.1 leu script sem BOM como ANSI e corrompeu acentos -> ancora ASCII e escrita UTF-8 sem BOM
- auditoria eletrica apontou GND sem referencia comum e rail 5 V ausente -> adicionei J_STAR_GND com loops, J_PSU_5V e fios de referencia, YAML passou de 25/26 para 27/34
- revisores pediram redacao honesta sobre unwrapMillis e FSM -> helper mantido, status T6 explicado e comando LIGHT separado da FSM

**Arquivos:**
- `firmware/laserseg/.opencode/skills/hardware-safety/SKILL.md`
- `firmware/laserseg/.opencode/skills/laserseg-context/SKILL.md`
- `firmware/laserseg/README.md`
- `firmware/laserseg/controller_dm556d/driver_config.h`
- `firmware/laserseg/docs/ESTADO.md`
- `firmware/laserseg/docs/HARDWARE-DECISIONS.md`
- `firmware/laserseg/docs/PROTOCOLO_UDP.md`
- `firmware/laserseg/docs/base-de-conhecimento-rastreamento-com-s3-cam-sim.md`
- `"firmware/laserseg/docs/example01/Base de conhecimento ÔÇö rastreamento com S3_CAM_SIM.md"`
- `"firmware/laserseg/docs/example01/Liga├º├Áes e ensaio.md"`
- `firmware/laserseg/docs/foco-luz-seguidor-001`
- `firmware/laserseg/docs/history.md`
- `firmware/laserseg/simulator/README.md`
- `firmware/laserseg/simulator/index.html`
- `firmware/laserseg/spec/SPEC-dm556d-nema23.md`
- `firmware/laserseg/spec/SPEC-simulador-web.md`
- `firmware/laserseg/tasks/todo.md`
- `firmware/laserseg/tests/test_simulator.py`
- `firmware/laserseg/docs/wireviz/`

### 2026-11-20 13:56 - chore(laserseg): fecha checklists do simulador web

**Area:** laserseg

**Feito:**
- todas as tarefas do plano e do todo do simulador web marcadas como concluidas apos implementacao validacao e push

**Dificuldades e solucoes:**
- nenhuma

**Arquivos:**
- `firmware/laserseg/tasks/plan-simulador-web.md`
- `firmware/laserseg/tasks/todo-simulador-web.md`

### 2026-11-20 13:56 - feat(laserseg): simulador web V1

**Area:** laserseg

**Feito:**
- pagina index.html abre em file:// com abas Painel Wi-Fi e Protocolo sem servidor ou placa
- FSM SAFE TRACK MANUAL com comandos TRACK LIGHT MANUAL telemetria ACK watchdog 1200 ms e webcam opcional com alvo sintetico
- constantes espelhadas de parser.h e driver_config.h defendidas por teste host e senha da rede local fica so em memoria da pagina

**Dificuldades e solucoes:**
- testes do simulador falharam primeiro por arquivos ausentes RED confirmado antes da implementacao e ficaram verdes apos HTML CSS JS e README
- ao restaurar link o fault LINK persistia ate o proximo comando entao o handler agora chama acceptCommand e emite ACK limpo
- Node 26.13.1 disponivel permitiu node --check como gate de sintaxe

**Arquivos:**
- `firmware/laserseg/README.md`
- `firmware/laserseg/docs/ESTADO.md`
- `firmware/laserseg/docs/history.md`
- `firmware/laserseg/spec/SPEC-simulador-web.md`
- `firmware/laserseg/simulator/`
- `firmware/laserseg/tasks/plan-simulador-web.md`
- `firmware/laserseg/tasks/todo-simulador-web.md`
- `firmware/laserseg/tests/test_simulator.py`

### 2026-11-20 13:40 - docs(laserseg): spec do simulador web V1

**Area:** laserseg

**Feito:**
- propoe pagina local sem hardware com abas Painel Wi-Fi e Protocolo
- espelha constantes de parser.h e driver_config.h com teste host
- define premissas limites e criterios de aceite antes de qualquer codigo

**Dificuldades e solucoes:**
- skill local spec-driven-development nao carrega pela ferramenta skill -> li o SKILL.md diretamente e segui o gate spec aprovacao impl
- risco de ambiguidade sobre webcam e aba Protocolo -> decisoes padrao registradas na spec para aprovacao explicita

**Arquivos:**
- `firmware/laserseg/spec/SPEC-simulador-web.md`

### 2026-11-20 13:13 - docs(laserseg): versiona prototipos e spec wifi provisioning

**Area:** laserseg

**Feito:**
- move de docs/example para docs/example01 sem perda (18 arquivos byte-identicos)
- adiciona docs/example02/simcam_tracker com camera e controladoras DM556D DRV8834 ULN2003
- adiciona spec aprovada de provisionamento WiFi AP para STA com portal e NVS
- adiciona simcam_tracker (1).zip de referencia
- 92 testes host verdes antes do commit
- referencias de documentacao atualizadas para example01 e example02

**Dificuldades e solucoes:**
- spec pedia commit seletivo enquanto example01 e example02 estavam sujos -> usuario autorizou explicitamente incluir tudo neste commit
- risco de perda no move verificado por hash git dos 18 arquivos todos identicos
- credenciais nos novos docs revisadas -> apenas senhas publicas de AP de prototipo e da spec nenhuma credencial de rede real
- save.ps1 falhou ao varrer nomes Unicode escapados -> core.quotepath=false no config local do repo e nova execucao sem alterar o script

**Arquivos:**
- `firmware/laserseg/.opencode/agents/hardware-auditor.md`
- `firmware/laserseg/.opencode/agents/laserseg-dev.md`
- `firmware/laserseg/.opencode/skills/laserseg-context/SKILL.md`
- `firmware/laserseg/AGENTS.md`
- `firmware/laserseg/README.md`
- `firmware/laserseg/docs/ESTADO.md`
- `firmware/laserseg/docs/PROTOCOLO_UDP.md`
- `"firmware/laserseg/docs/example/Base de conhecimento ÔÇö rastreamento com S3_CAM_SIM.md"`
- `"firmware/laserseg/docs/example/Liga├º├úo de hardware ÔÇö T-SIMCam IR-CUT, pan-tilt e foco LED.md"`
- `"firmware/laserseg/docs/example/Liga├º├úo revisada ÔÇö S3_CAM_SIM V1.3 + pan-tilt + iluminador LED.md"`
- `"firmware/laserseg/docs/example/Liga├º├Áes e ensaio.md"`
- `"firmware/laserseg/docs/example/Projeto de laser seguidor com LilyGO T-SIMCAM ESP32-S3.zip"`
- `"firmware/laserseg/docs/example/Projeto inicial_ rastreador pan-tilt com LILYGO T-SIMCam IR-CUT.md"`
- `"firmware/laserseg/docs/example/Projetos GitHub semelhantes_ c├ómera rastreadora pan-tilt.md"`
- `firmware/laserseg/docs/example/REFERENCIAS.md`
- `"firmware/laserseg/docs/example/SIMCAM Tracker ÔÇö Arduino CLI + pan-tilt.md"`
- `firmware/laserseg/docs/example/camera_node.ino`
- `firmware/laserseg/docs/example/camera_pins.h`
- `firmware/laserseg/docs/example/compile_all.sh`
- `firmware/laserseg/docs/example/controller_drv8834.ino`
- `firmware/laserseg/docs/example/controller_uln2003.ino`
- `firmware/laserseg/docs/example/pan_tilt_parametrico.scad`
- `firmware/laserseg/docs/example/pasted_file_gR9fDH_image.png`
- `firmware/laserseg/docs/example/simcam_tracker.zip`
- `firmware/laserseg/docs/example/simcam_tracking_context.json`
- `firmware/laserseg/docs/history.md`
- `firmware/laserseg/docs/example01/`
- `firmware/laserseg/docs/example02/`
- `"firmware/laserseg/docs/simcam_tracker (1).zip"`
- `firmware/laserseg/spec/SPEC-wifi-provisioning.md`

### 2026-11-19 17:19 - docs(laserseg): nota de verificacao de hardware T1

**Area:** laserseg

**Feito:**
- criado docs/HARDWARE-DECISIONS.md com as 7 respostas da spec e o grau de confirmacao de cada uma
- dados de variantes HANPOSE/Leadshine/StepperOnline registrados como contexto, nao como especificacao do Azanelli
- credencial Wi-Fi de bancada nao transcrita no repositorio; apenas registrada como pendencia de remocao
- foco-luz-seguidor-001 passa a referenciar o novo documento e avisa que T5/T6 continuam bloqueados
- tasks/todo.md atualizado com os itens parciais e bloqueados

**Dificuldades e solucoes:**
- fontes publicas divergem sobre semantica de SW4 e tabela de micropasso -> cada divergencia foi registrada com a variante de origem, sem generalizar
- risco de transformar "2,8 A" de variante comercial em dado da etiqueta -> status marcado como parcial e foto da etiqueta ficou como acao pendente
- nota do usuario continha SSID/senha de bancada -> valores omitidos do documento para nao introduzir segredo no repo

**Arquivos:**
- `firmware/laserseg/docs/HARDWARE-DECISIONS.md`
- `firmware/laserseg/docs/foco-luz-seguidor-001`
- `firmware/laserseg/tasks/todo.md`
- `docs/DEVLOG.md`

### 2026-11-19 17:03 - feat(laserseg): no DM556D serial-only com nucleo compartilhado

**Area:** laserseg

**Feito:**
- spec DM556D + NEMA 23 marcada como aprovada; plano e tasks registrados
- nucleo de protocolo nao duplicado: novo sketch inclui parser.cpp e autotest.h de controller/
- constantes da spec em tools/motion_config.py e controller_dm556d/driver_config.h, com static_assert contra o nucleo
- unwrapMillis() prova em compile time a fronteira do watchdog apos wrap do millis()
- 8 testes novos de movimento -> 92 testes host verdes; vetores regerados byte a byte identicos
- perfil laserseg_dm556d em tools/flash.ps1; compile exit=0; placa detectada e gravada; AUTOTEST 76/76 PASS
- nenhum GPIO, motor, luz ou WiFi tocado (LASERSEG_ENABLE_GPIO=0)

**Dificuldades e solucoes:**
- risco de duplicar a tabela de decisao -> novo no inclui os arquivos de controller/ por caminho relativo, sem copiar
- wrap do millis() poderia burlar o watchdog de 1200 ms -> tempo monotonico int64 no no + static_assert da transicao 2^32-100 -> 1101
- reviewer apontou que so a replica Python testava o wrap -> prova movida para tempo de compilacao no propriao cabecalho C++
- save.ps1 usa git add -A e incluiria as alteracoes do usuario em docs/example01/example02 -> commit seletivo explicito

**Pendencias registradas para T5/T6:**
- tratar AutoTestResult.failed != 0 como falha fechada antes de habilitar GPIO
- conectar nowMs() a Session::feed()/poll() quando o UDP entrar
- 7 respostas de hardware + hardware-auditor antes de qualquer saida fisica

**Arquivos:**
- `firmware/laserseg/spec/SPEC-dm556d-nema23.md`
- `firmware/laserseg/tasks/plan.md`
- `firmware/laserseg/tasks/todo.md`
- `firmware/laserseg/tools/motion_config.py`
- `firmware/laserseg/tests/test_motion.py`
- `firmware/laserseg/controller_dm556d/controller_dm556d.ino`
- `firmware/laserseg/controller_dm556d/core.cpp`
- `firmware/laserseg/controller_dm556d/driver_config.h`
- `tools/flash.ps1`
- `docs/DEVLOG.md`

### 2026-11-19 15:32 - fix(laserseg): telemetria em maiusculas validada na placa

**Area:** laserseg

**Feito:**
- telemetria usa mapa proprio NONE RANGE LINK em maiusculas
- autoteste mostra expected e got em caso de falha
- validacao na placa AUTOTEST PASS 76/76
- docs atualizadas

**Dificuldades e solucoes:**
- bug so apareceu no dispositivo -> reasonName retornava minusculas e foi separado de telemetryFaultName
- falha do autoteste era opaca -> checkTelemetry imprime esperado e obtido

**Arquivos:**
- `firmware/laserseg/controller/autotest.h`
- `firmware/laserseg/controller/parser.cpp`
- `firmware/laserseg/docs/ESTADO.md`
- `firmware/laserseg/docs/history.md`

### 2026-11-19 15:28 - feat(laserseg): parser UDP C++ com autoteste e telemetria

**Area:** laserseg

**Feito:**
- controller/ com parser e state machine espelhando protocol_ref.py
- autoteste com 55 vetores gerados da referencia Python
- telemetria ACK seq state fault light documentada
- perfil laserseg repontado para controller e docs atualizadas

**Dificuldades e solucoes:**
- bit valid retornava format em vez de range -> motivo passou a vir de reason como na referencia
- autoteste de reinicio de sequencia era falso positivo -> T,1 / TRACK,1 / T,1 com controle negativo
- revisor apontou perfil desatualizado -> repontado e recompilado com exit=0

**Arquivos:**
- `AGENTS.md`
- `firmware/laserseg/AGENTS.md`
- `firmware/laserseg/docs/ESTADO.md`
- `firmware/laserseg/docs/PROTOCOLO_UDP.md`
- `firmware/laserseg/docs/history.md`
- `tools/flash.ps1`
- `firmware/laserseg/controller/`
- `firmware/laserseg/tools/gen_autotest_vectors.py`

### 2026-11-19 14:24 - chore: remove pycache versionado do parser

**Area:** tooling

**Feito:**
- gitignore cobre __pycache__ e pyc
- runner desativa bytecode
- artifacts removidos do indice

**Dificuldades e solucoes:**
- save.ps1 commitou pyc gerado pelos testes -> git rm --cached e gitignore corrigido no mesmo bloco

**Arquivos:**
- `.gitignore`
- `firmware/laserseg/tests/__pycache__/test_protocol.cpython-314.pyc`
- `firmware/laserseg/tests/run.ps1`
- `firmware/laserseg/tools/__pycache__/protocol_ref.cpython-314.pyc`

### 2026-11-19 14:23 - test(laserseg): parser udp validado e probe serial versionado

**Area:** laserseg, tooling

**Feito:**
- contrato v0.1 do parser com 84 testes no host
- idade aplicada a todos os comandos e watchdog com parada explicita dos eixos
- probe serial versionado e perfil laserseg compilado com exit 0
- mqtt local websocket quick tunnel e documentacao operacional

**Dificuldades e solucoes:**
- revisao independente achou idade so em T e watchdog sem parada de eixos -> corrigido com axes_stopped e testes de regressao
- perfil laserseg apontava para sketch inexistente -> probe versionado
- arduino-cli exige ino com nome da pasta -> renomeado e compile confirmado
- firewall e servico elevado bloqueados por UAC -> pendencia registrada para o usuario

**Arquivos:**
- `AGENTS.md`
- `firmware/laserseg/AGENTS.md`
- `firmware/laserseg/README.md`
- `firmware/laserseg/docs/ESTADO.md`
- `firmware/laserseg/docs/PROTOCOLO_UDP.md`
- `firmware/laserseg/docs/history.md`
- `tools/flash.ps1`
- `firmware/laserseg/docs/MQTT.md`
- `firmware/laserseg/serial_probe/`
- `firmware/laserseg/tests/`
- `firmware/laserseg/tools/`
- `tools/mqtt/`

### 2026-11-19 12:23 - docs(laserseg): path do python aponta para a versao lts

**Area:** laserseg

**Feito:**
- AGENTS.md atualizado para registrar que o PATH do usuario agora resolve python 3.14.10 e nao o stub da microsoft store
- backup do PATH original preservado em %TEMP%/opencode/backups

**Dificuldades e solucoes:**
- stub da microsoft store tinha precedencia no PATH -> reordenei o PATH do usuario com backup e validei python --version e pyyaml em processo recarregado

**Arquivos:**
- `firmware/laserseg/AGENTS.md`

### 2026-11-19 12:22 - chore(laserseg): skill global, python lts e auditoria de frontmatter

**Area:** laserseg

**Feito:**
- skill espressif-lilygo promovida a global com mesmo SHA-256 e regra de sincronizacao entre as duas copias
- python 3.14.10 LTS instalado no lugar da 3.15.2 e do stub da microsoft store com pyyaml 6.0.3 e pyserial 3.5
- auditoria real de frontmatter com pyyaml em 249 skills com zero erros e regra de LTS registrada em AGENTS e history
- README ESTADO THIRD_PARTY e history atualizados

**Dificuldades e solucoes:**
- validador node dava falso positivo em arquivos CRLF -> normalizei BOM e CRLF e depois troquei por parser pyyaml real com python lts
- winget indisponivel e python.exe era stub da store -> baixei instalador oficial 3.14.10 e desinstalei 3.15.2 pelo uninstaller em cache
- risco de sobrescrever skill global -> destino verificado vazio e backup dos arquivos editados em %TEMP%/opencode/backups

**Arquivos:**
- `firmware/laserseg/.opencode/THIRD_PARTY.md`
- `firmware/laserseg/AGENTS.md`
- `firmware/laserseg/README.md`
- `firmware/laserseg/docs/ESTADO.md`
- `firmware/laserseg/docs/history.md`

### 2026-11-19 12:13 - feat(laserseg): especialistas esp32 e lilygo

**Area:** laserseg

**Feito:**
- skill local espressif-lilygo com pinagem oficial T-SimCam V1.3/V1.4, core 3.3.11, particao app3M_fat9M_16MB e regras de gravacao
- 39 skills esp-* instaladas globalmente de grantwilk/espressif-skills MIT e descobertas pelo OpenCode
- AGENTS.md, README, ESTADO, THIRD_PARTY e history atualizados com fontes e especialistas

**Dificuldades e solucoes:**
- websearch sem resultados -> consultei a API do GitHub e encontrei espressif/skills e grantwilk/espressif-skills
- repositorio oficial espressif/skills estava vazio -> usei skills atomicas da comunidade com licenca MIT e registrei a ressalva
- risco de sobrescrever arquivos -> backup previo em %TEMP%/opencode/backups antes de editar

**Arquivos:**
- `firmware/laserseg/.opencode/THIRD_PARTY.md`
- `firmware/laserseg/AGENTS.md`
- `firmware/laserseg/README.md`
- `firmware/laserseg/docs/ESTADO.md`
- `firmware/laserseg/docs/history.md`
- `firmware/laserseg/.opencode/skills/espressif-lilygo/`

### 2026-11-19 12:09 - docs(laserseg): regra de backup e migrations seguras

**Area:** laserseg

**Feito:**
- history.md: regras 7 e 8 sobre backup obrigatorio antes de sobrescrever/apagar codigo e cuidado com migrations destrutivas
- AGENTS.md: secao Backup e banco de dados com backup previo, diff, rollback e confirmacao explicita para DROP/TRUNCATE/DELETE

**Dificuldades e solucoes:**
- nenhuma -> apenas documentacao das diretrizes recebidas

**Arquivos:**
- `firmware/laserseg/AGENTS.md`
- `firmware/laserseg/docs/history.md`

### 2026-11-19 12:03 - docs(laserseg): fundacao com skills e agentes

**Area:** laserseg

**Feito:**
- README, ESTADO, PROTOCOLO_UDP e history.md documentados
- 4 skills proprios e 9 skills instalados do addyosmani/agent-skills com licenca MIT
- 3 agentes OpenCode e opencode.jsonc com context7 e github desabilitado
- AGENTS.md com mapeamento intencao skill e regras de seguranca

**Dificuldades e solucoes:**
- npx bloqueado pela politica do PowerShell -> clone manual e copia preservando caminhos relativos
- arduino-cli ausente e laserseg sem perfil no flash.ps1 -> build e flash registrados como bloqueio
- mcp list do CLI nao mostra servers de projeto -> validacao com debug config e debug agents

**Arquivos:**
- `firmware/laserseg/.opencode/THIRD_PARTY.md`
- `firmware/laserseg/.opencode/THIRD_PARTY_LICENSES/addyosmani-agent-skills-LICENSE.txt`
- `firmware/laserseg/.opencode/agents/hardware-auditor.md`
- `firmware/laserseg/.opencode/agents/laserseg-dev.md`
- `firmware/laserseg/.opencode/agents/laserseg-reviewer.md`
- `firmware/laserseg/.opencode/opencode.jsonc`
- `firmware/laserseg/.opencode/references/definition-of-done.md`
- `firmware/laserseg/.opencode/references/performance-checklist.md`
- `firmware/laserseg/.opencode/references/security-checklist.md`
- `firmware/laserseg/.opencode/references/testing-patterns.md`
- `firmware/laserseg/.opencode/skills/code-review-and-quality/SKILL.md`
- `firmware/laserseg/.opencode/skills/constraint-driven-development/SKILL.md`
- `firmware/laserseg/.opencode/skills/constraint-driven-development/references/floor-guard.md`
- `firmware/laserseg/.opencode/skills/constraint-driven-development/scripts/quality-gate.mjs`
- `firmware/laserseg/.opencode/skills/debugging-and-error-recovery/SKILL.md`
- `firmware/laserseg/.opencode/skills/documentation-and-adrs/SKILL.md`
- `firmware/laserseg/.opencode/skills/esp32-build-workflow/SKILL.md`
- `firmware/laserseg/.opencode/skills/hardware-safety/SKILL.md`
- `firmware/laserseg/.opencode/skills/incremental-implementation/SKILL.md`
- `firmware/laserseg/.opencode/skills/laserseg-context/SKILL.md`
- `firmware/laserseg/.opencode/skills/planning-and-task-breakdown/SKILL.md`
- `firmware/laserseg/.opencode/skills/security-and-hardening/SKILL.md`
- `firmware/laserseg/.opencode/skills/security-and-hardening/references/hardening-patterns.md`
- `firmware/laserseg/.opencode/skills/spec-driven-development/SKILL.md`
- `firmware/laserseg/.opencode/skills/test-driven-development/SKILL.md`
- `firmware/laserseg/.opencode/skills/udp-protocol/SKILL.md`
- `firmware/laserseg/.opencode/vendor-agents/code-reviewer.md`
- `firmware/laserseg/.opencode/vendor-agents/security-auditor.md`
- `firmware/laserseg/.opencode/vendor-agents/test-engineer.md`
- `firmware/laserseg/.skills/SKILL-001.md`
- `firmware/laserseg/AGENTS.md`
- `firmware/laserseg/README.md`
- `firmware/laserseg/docs/ESTADO.md`
- `firmware/laserseg/docs/PROTOCOLO_UDP.md`
- `firmware/laserseg/docs/base-de-conhecimento-rastreamento-com-s3-cam-sim.md`
- `"firmware/laserseg/docs/example/Base de conhecimento \342\200\224 rastreamento com S3_CAM_SIM.md"`
- `"firmware/laserseg/docs/example/Liga\303\247\303\243o de hardware \342\200\224 T-SIMCam IR-CUT, pan-tilt e foco LED.md"`
- `"firmware/laserseg/docs/example/Liga\303\247\303\243o revisada \342\200\224 S3_CAM_SIM V1.3 + pan-tilt + iluminador LED.md"`
- `"firmware/laserseg/docs/example/Liga\303\247\303\265es e ensaio.md"`
- `"firmware/laserseg/docs/example/Projeto de laser seguidor com LilyGO T-SIMCAM ESP32-S3.zip"`
- `"firmware/laserseg/docs/example/Projeto inicial_ rastreador pan-tilt com LILYGO T-SIMCam IR-CUT.md"`
- `"firmware/laserseg/docs/example/Projetos GitHub semelhantes_ c\303\242mera rastreadora pan-tilt.md"`
- `firmware/laserseg/docs/example/REFERENCIAS.md`
- `"firmware/laserseg/docs/example/SIMCAM Tracker \342\200\224 Arduino CLI + pan-tilt.md"`
- `firmware/laserseg/docs/example/camera_node.ino`
- `firmware/laserseg/docs/example/camera_pins.h`
- `firmware/laserseg/docs/example/compile_all.sh`
- `firmware/laserseg/docs/example/controller_drv8834.ino`
- `firmware/laserseg/docs/example/controller_uln2003.ino`
- `firmware/laserseg/docs/example/pan_tilt_parametrico.scad`
- `firmware/laserseg/docs/example/pasted_file_gR9fDH_image.png`
- `firmware/laserseg/docs/example/simcam_tracker.zip`
- `firmware/laserseg/docs/example/simcam_tracking_context.json`
- `firmware/laserseg/docs/foco-luz-seguidor-001`
- `firmware/laserseg/docs/simcam_tracking_context.json`
- `firmware/laserseg/docs/history.md`

### 2026-11-18 17:38 - feat(msa): lista redes wifi no modo cliente

**Area:** msa

**Feito:**
- Campo de senha deixou de vazar para tras do card de limites ao remover min-height zero do card
- Novo endpoint GET /api/wifi/scan varre ate 24 SSIDs unicos ordenados por sinal
- Aba Config lista redes com RSSI canal e criptografia e permite digitar outro SSID
- WebAssets e cache do service worker atualizados

**Dificuldades e solucoes:**
- Card com min-height zero encolhia abaixo do conteudo e o input ficava atras do card vizinho -> removi o min-height mantendo min-width zero
- Varias redes repetidas e fracas podiam poluir a lista -> dedupliquei por SSID e ordenei por RSSI
- Cliente pode querer rede oculta ou fora da varredura -> adicionei opcao Digitar SSID

**Arquivos:**
- `firmware/msa/DashboardService.cpp`
- `firmware/msa/DashboardService.h`
- `firmware/msa/README.md`
- `firmware/msa/WebAssets.h`
- `firmware/msa/web/css/style.css`
- `firmware/msa/web/index.html`
- `firmware/msa/web/js/app.js`
- `firmware/msa/web/sw.js`

### 2026-11-18 17:23 - fix(msa): ajusta interface mobile first

**Area:** msa

**Feito:**
- Unidade base mobile first de 9 a 14px com reducao controlada em paisagem
- Layout em uma coluna no celular e breakpoints min-width para tablet e desktop
- Fontes minimas 16px nos campos, alvos de toque de 44px e areas seguras do iOS
- Cache do service worker e WebAssets regenerados e regravados em COM12

**Dificuldades e solucoes:**
- O clamp antigo min(1vw,2vh) caia para 5px no retrato -> troquei por 2.6vw com piso de 9px e excecao em landscape
- CSS usava breakpoints max-width e escondia grids no mobile -> reescrevi mobile first com min-width
- Service worker podia servir css antigo -> bump de style.css?v=3 e cache msa-v3

**Arquivos:**
- `firmware/msa/WebAssets.h`
- `firmware/msa/web/css/style.css`
- `firmware/msa/web/index.html`
- `firmware/msa/web/sw.js`

### 2026-11-18 15:28 - feat(msa): adiciona OLED e interface responsiva

**Area:** msa, raiz

**Feito:**
- DisplayService SSD1306 com deteccao automatica 0x3C/0x3D e leituras/avisos no OLED
- Interface web reescrita no padrao irobot com PWA, tela cheia, ticker e layout responsivo
- Configuracao Wi-Fi igual irobot com AP personalizavel, STA e reinicio ao salvar
- Compilacao, gravacao em COM12 e serial validando AHT20 0x38, BMP280 0x77 e OLED 0x3C

**Dificuldades e solucoes:**
- Adafruit SSD1306 nao estava instalada e o display podia responder em 0x3C ou 0x3D -> instalei a lib 2.5.17 e fiz scan I2C antes do begin com periphBegin=false para nao reiniciar o Wire
- AP do irobot so salvava SSID/senha sem aplicar -> persisti apSsid/apPass no NVS e carrego no boot antes do softAP
- Relatorio de exploracao veio truncado -> revisei diretamente web/js/app.js, index.html e style.css do irobot para replicar a tecnologia

**Arquivos:**
- `AGENTS.md`
- `README.md`
- `firmware/msa/DashboardService.cpp`
- `firmware/msa/DashboardService.h`
- `firmware/msa/HardwareConfig.h`
- `firmware/msa/README.md`
- `firmware/msa/WebAssets.h`
- `firmware/msa/msa.ino`
- `firmware/msa/web/css/style.css`
- `firmware/msa/web/index.html`
- `firmware/msa/web/js/app.js`
- `firmware/msa/web/manifest.json`
- `firmware/msa/web/sw.js`
- `firmware/msa/DisplayService.cpp`
- `firmware/msa/DisplayService.h`

### 2026-11-18 14:31 - feat(msa): cria firmware de sensores ambientais

**Area:** msa, raiz, tooling

**Feito:**
- novo firmware msa com AHT20 e BMP280 no I2C, leitura validada e cadencia configuravel
- dashboard web, API REST, alertas com tendencia, NVS, Wi-Fi AP/STA e mDNS
- perfil msa no flash.ps1 e correcao do detectOnly com stderr do esptool
- documentacao de hardware e API e README/AGENTS atualizados
- compilacao verificada com arduino-cli

**Dificuldades e solucoes:**
- detectOnly abortava porque o esptool escreve no stderr com ErrorActionPreference Stop -> alterei apenas o escopo da chamada
- tres portas seriais conectadas -> identifiquei o ESP32 pelo esptool e mantive deteccao dinamica
- WebAssets.h e gerado -> gerei apos editar web/ antes de compilar

**Arquivos:**
- `AGENTS.md`
- `README.md`
- `tools/flash.ps1`
- `firmware/msa/`

### 2026-11-18 08:43 - fix(irobot): corrige ordem dos botoes do D-pad

**Area:** irobot

**Feito:**
- Home e DL com Tras abaixo de Frente e Direita abaixo de Esquerda
- preview sincronizado
- WebAssets regenerado
- compilacao validada em 85% de flash

**Dificuldades e solucoes:**
- ordem vinha do fluxo automatico do grid com display contents -> reordenado o DOM dos botoes e medido com Chrome headless

**Arquivos:**
- `firmware/irobot/WebAssets.h`
- `firmware/irobot/preview/index.html`
- `firmware/irobot/web/index.html`

### 2026-11-18 08:35 - docs: consolida licoes aprendidas do iRobot e Windfollow

**Area:** docs, raiz

**Feito:**
- novo documento docs/LICOES_APRENDIDAS.md com hardware navegacao UI responsiva PWA geracao de assets preview workflow e checklist
- registro de erros e correcoes como virgula no WebAssets D-pad cortado knob fixo build versionado e limitacao PWA
- README aponta para o novo documento
- aprendizados tambem salvos no grafo de memoria persistente

**Dificuldades e solucoes:**
- conhecimento estava disperso entre conversas commits e DEVLOG -> consolidado em um documento unico e no grafo de memoria

**Arquivos:**
- `README.md`
- `docs/LICOES_APRENDIDAS.md`

### 2026-11-17 22:28 - feat(irobot): layout Home responsivo em retangulo perfeito

**Area:** irobot, raiz

**Feito:**
- grid preenche a viewport exatamente sem scroll em 667x375 844x390 e 960x540
- fontes botoes cards e joystick escalam por unidade de viewport
- comandos de autonomo e velocidade foram para a faixa inferior junto de sensores e status
- densidade compacta em paisagem baixa evita corte no D-pad
- raio do knob do joystick agora e medido do DOM
- preview sincronizado e README atualizado

**Dificuldades e solucoes:**
- texto do D-pad cortava em 844x390 -> medi retangulos com navegador headless -> media query compacta aplicada
- raio fixo do knob quebrava com joystick escalado -> raio agora lido do elemento renderizado

**Arquivos:**
- `README.md`
- `firmware/irobot/WebAssets.h`
- `firmware/irobot/preview/app.js`
- `firmware/irobot/preview/index.html`
- `firmware/irobot/preview/style.css`
- `firmware/irobot/web/css/style.css`
- `firmware/irobot/web/index.html`
- `firmware/irobot/web/js/app.js`

### 2026-11-17 21:51 - feat(irobot): ajusta UI Home, icones PWA e preview local

**Area:** irobot, raiz

**Feito:**
- avisos ambientais agora aparecem no centro do cabecalho ao lado do titulo iRobot
- sensores e status foram movidos para baixo dos botoes de direcao na Home
- favicon e icones PWA PNG 32/192/512 servidos pelo firmware e embutidos no WebAssets
- manifest atualizado com display fullscreen e service worker v7
- pasta preview com mock de API e cenarios alerts/fire/stuck para validar a tela no PC
- artefatos de build removidos do git e build/ adicionado ao gitignore
- README atualizado com instrucoes do preview

**Dificuldades e solucoes:**
- gerador de assets escrevia bytes PNG sem virgula entre linhas -> compile falhou -> join com virgula aplicado e compile ok 85% flash
- PWA em AP HTTP nao oferece instalacao real no navegador -> documentado HTTPS/localhost e manifest com PNGs corretos

**Arquivos:**
- `.gitignore`
- `README.md`
- `firmware/irobot/DashboardService.cpp`
- `firmware/irobot/DashboardService.h`
- `firmware/irobot/WebAssets.h`
- `firmware/irobot/build/esp32.esp32.esp32/build.options.json`
- `firmware/irobot/build/esp32.esp32.esp32/flash_args`
- `firmware/irobot/build/esp32.esp32.esp32/partitions.csv`
- `firmware/irobot/build/esp32.esp32.esp32/sdkconfig`
- `firmware/irobot/tools/generate_web_assets.ps1`
- `firmware/irobot/web/css/style.css`
- `firmware/irobot/web/index.html`
- `firmware/irobot/web/js/app.js`
- `firmware/irobot/web/manifest.json`
- `firmware/irobot/web/sw.js`
- `firmware/irobot/preview/`
- `firmware/irobot/tools/generate_icons.ps1`
- `firmware/irobot/web/favicon.png`
- `firmware/irobot/web/icon-192.png`
- `firmware/irobot/web/icon-512.png`

### 2026-11-17 21:12 - docs: README profissional com todos os projetos, hardware, API e fluxo de trabalho

**Area:** raiz

**Feito:**
- README reescrito com badges, autor Tiago de Abreu @devtiagoabreu, visao geral dos 3 projetos
- documentado hardware, GPIOs, rede, modulos, comandos seriais e navegacao autonoma do iRobot
- adicionada secao de ferramentas flash/assets/save e simuladores
- secao PWA com aviso HTTPS

**Dificuldades e solucoes:**
- nenhuma

**Arquivos:**
- `README.md`

### 2026-11-17 21:04 - feat(irobot): nova UI web com abas Home/DL/Config, PWA e letreiro de ambiente

**Area:** irobot

**Feito:**
- index.html reescrito como SPA com abas Home (paisagem joystick esquerda botoes centro sensores/status inferior badge conectado), DL (tela antiga mantida) e Config (wifi AP/client e PWM)
- app.js unificado com joystick duplo, D-pad, autonomo, PWA service worker e fullscreen
- manifest.json favicon.svg sw.js adicionados e servidos
- DashboardService rotas manifest favicon sw api/wifi restart
- irobot.ino conecta STA quando salvo

**Dificuldades e solucoes:**
- PWA em HTTP no AP nao instala em navegadores (exige HTTPS ou localhost): mantido manifest e service worker para quando servir via HTTPS e orientacao fullscreen
- layouts responsivos e joystick duplo exigiram rewrite do app.js

**Arquivos:**
- `firmware/irobot/DashboardService.cpp`
- `firmware/irobot/DashboardService.h`
- `firmware/irobot/WebAssets.h`
- `firmware/irobot/build/esp32.esp32.esp32/build.options.json`
- `firmware/irobot/irobot.ino`
- `firmware/irobot/tools/generate_web_assets.ps1`
- `firmware/irobot/web/css/style.css`
- `firmware/irobot/web/index.html`
- `firmware/irobot/web/js/app.js`
- `firmware/irobot/web/favicon.svg`
- `firmware/irobot/web/manifest.json`
- `firmware/irobot/web/sw.js`

### 2026-11-17 20:40 - feat(irobot): reconhecimento de 360 graus e alinhamento na direcao mais livre no modo autonomo

**Area:** irobot

**Feito:**
- novo estado SCAN varre 360 graus parando e medindo a cada 60 graus
- leituras sem alvo gravam distancia maxima
- apos scan o robo gira para a direcao mais livre e segue em frente
- heading estimado por tempo de giro
- estados SCAN_MEASURE SCAN_TURN ALIGN_BEST adicionados

**Dificuldades e solucoes:**
- nenhuma

**Arquivos:**
- `firmware/irobot/AutoNavigator.cpp`
- `firmware/irobot/AutoNavigator.h`
- `firmware/irobot/build/`

### 2026-11-15 23:59 - docs(irobot): documentar sensores, avisos ambientais e navegacao autonoma

**Area:** docs, irobot, raiz

**Feito:**
- README do firmware irobot reescrito como referencia completa (hardware e alimentacao, tabela dos sensores I2C com enderecos e cadencia, maquina de estados do autonomo com tabela de constantes, regras dos avisos ambientais, API com exemplo real de /api/sensors, comandos seriais incluindo a e m, compilacao e gravacao, tabela de diagnostico de problemas, arquivos, prototipos, limites e proximos passos de hardware) e novo documento docs/IROBOT_NAVEGACAO_E_SENSORES.md com diagramas mermaid do fluxo de dados e da FSM, justificativa do pivo em vez de arco, tratamento de leituras invalidas do VL53L0X, criterios de calibracao, evidencias dos testes seriais, logica detalhada de previsao de chuva por barometro e de incendio, mitigacoes de brownout e checklist de teste, alem de links no README raiz

**Dificuldades e solucoes:**
- Manter a documentacao fiel ao codigo exigiu conferir cada valor no HardwareConfig e nas classes novas (cadencia 80/400 ms, janela de 3 h com 180 amostras, limiares e histereses) e registrar o que ainda e apenas planejado, como o monitor de bateria e os sensores de chuva e chama, para nao prometer recurso que o firmware nao tem

**Arquivos:**
- `README.md`
- `firmware/irobot/README.md`
- `docs/IROBOT_NAVEGACAO_E_SENSORES.md`

### 2026-11-15 23:28 - feat(irobot): avisos ambientais (chuva, calor, frio, ar seco, incendio)

**Area:** irobot

**Feito:**
- Novo EnvironmentAdvisor com historico de pressao do BMP280 (1 amostra por minuto, 3 h) para prever chuva pela regra de barometro e janela de 1 min da temperatura para detectar subida rapida, gerando avisos de chuva provavel, calor acima de 26 C, frio abaixo de 17 C, umidade abaixo de 30 e possivel incendio com histerese, expostos no objeto alerts de /api/sensors e no serial pelo comando m, com card Avisos e banner vermelho de incendio no painel e linha de avisos na pagina de teste mais suporte opcional a sensor de chuva FC-37 no GPIO34 e chama KY-026 no GPIO39 por defines, e o sensor ToF continua sendo lido durante o giro com log a cada 500 ms e leitura vencida nao conta mais como caminho livre

**Dificuldades e solucoes:**
- A previsao de chuva depende de 30 min de historico de pressao, entao no arranque o app mostra coletando dados em vez de arriscar um falso negativo, e o giro sem encoder precisou de um limite de idade da amostra (500 ms) para nao decidir caminho livre com leitura velha enquanto o robo roda

**Arquivos:**
- `firmware/irobot/AutoNavigator.cpp`
- `firmware/irobot/AutoNavigator.h`
- `firmware/irobot/DashboardService.cpp`
- `firmware/irobot/DashboardService.h`
- `firmware/irobot/HardwareConfig.h`
- `firmware/irobot/README.md`
- `firmware/irobot/SensorService.cpp`
- `firmware/irobot/SensorService.h`
- `firmware/irobot/WebAssets.h`
- `firmware/irobot/irobot.ino`
- `firmware/irobot/web/css/style.css`
- `firmware/irobot/web/index.html`
- `firmware/irobot/web/js/app.js`
- `firmware/irobot/web/teste.html`
- `firmware/irobot/EnvironmentAdvisor.cpp`
- `firmware/irobot/EnvironmentAdvisor.h`

### 2026-11-15 23:15 - feat(irobot): desvio autonomo com re, pivo e espera por 1 m livre

**Area:** irobot

**Feito:**
- AutoNavigator para a 50 cm do obstaculo (era 25 cm), recua 500 ms antes de girar (800 ms a cada 3 obstaculos) e gira no proprio eixo com uma roda em re e outra em frente, em vez do arco que derrapava
- o giro nao tem mais tempo fixo, continua ate o ToF ler 1 m livre em 2 leituras seguidas, com minimo de 450 ms e maximo de 2,4 s por segmento
- se nao achar saida recua 350 ms e continua girando do mesmo lado e, apos dois segmentos, troca de lado
- so retoma o reto depois de assentar e confirmar 1 m
- cada giro sem saida tambem conta como tentativa para o detector de enroscamento (100 tentativas) e moveDifferential agora atualiza a direcao para a telemetria

**Dificuldades e solucoes:**
- O FSM antigo alternava lados a cada 650 ms com um arco de roda unica, o que num chassi com roda traseira fixa virava derrapagem e nunca dava tempo de medir
- reescrevi como maquina de estados inspirada nos projetos de obstacle avoidance do GitHub (stop-backup-pivot-turnate-clear) e observei no serial que, numa bancada cercada a 15 cm, ele agora alterna re e giro e troca de lado em vez de ficar piscando entre esquerda e direita

**Arquivos:**
- `firmware/irobot/AutoNavigator.cpp`
- `firmware/irobot/AutoNavigator.h`
- `firmware/irobot/HardwareConfig.h`
- `firmware/irobot/MotorEngine.cpp`
- `firmware/irobot/SensorService.h`

### 2026-11-15 22:12 - fix(irobot): boot sem acionar motores e Wi-Fi com menor potencia de TX

**Area:** irobot

**Feito:**
- GPIO27 e GPIO14 agora sao colocados em LOW na primeira linha do setup, antes de serial e Wi-Fi, evitando que ENA/ENB flutuando no boot girem os motores e derrubem a alimentacao
- AP reduzido para 11 dBm para diminuir o pico de corrente
- boot avisa quando o reset anterior foi brownout e explica o que revisar
- diagnostico confirmou que no USB o AP sobe normal (Irobot-Setup) e o problema é alimentacao/bateria

**Dificuldades e solucoes:**
- O sintoma nao reproduziu com USB (AP sobe e fica estavel 15 min com heap em 207 kB e sensor lendo distancia), mas o usuario confirmou que na bateria a rede nem aparece -> sem banco de testes com bateria, apliquei mitigacoes de firmware (enable em LOW no boot, TX 11 dBm) e documentei a causa provavel: sag do 5V do L298N e ENA/ENB flutuando durante o boot

**Arquivos:**
- `firmware/irobot/irobot.ino`

### 2026-11-15 21:50 - chore(irobot): diagnostico de reset, heap e clientes WiFi

**Area:** irobot

**Feito:**
- Boot agora imprime o motivo do reset anterior (incluindo BROWNOUT) via esp_reset_reason
- heartbeat de 10 s mostra heap livre, menor heap, numero de clientes conectados ao AP, estado do autonomo e distancia
- Wi-Fi do AP com setSleep(false) para nao economizar energia
- teste de 40 s com motores em 150 PWM sem reset e heap estavel em 207 kB

**Dificuldades e solucoes:**
- O sintoma relatado (AP some e nao volta) nao reproduziu em bancada com motores ligados -> adicionei instrumentacao de reset e heap e deixei uma captura serial longa rodando para pegar a causa no momento da falha

**Arquivos:**
- `firmware/irobot/irobot.ino`

### 2026-11-15 21:32 - chore(irobot): gravacao no COM9, diagnostico de distancia bruta e mDNS sem duplicata

**Area:** irobot

**Feito:**
- Gravado no ESP32 D0WDQ6 do COM9 (1.074.303 bytes, 81% flash)
- comandos seriais a e m validados no hardware com autonomo entrando em frente e desligando corretamente
- AHT20 e BMP280 respondendo (26,8 C 70,4% 945,5 hPa)
- serial m agora mostra a leitura bruta do VL53L0X (8191 significa nenhum alvo no alcance)
- removida a segunda chamada de MDNS.begin duplicada no DashboardService que gerava o erro service already exists

**Dificuldades e solucoes:**
- A distancia aparecia 0 e nao dava para saber se o sensor estava quebrado ou apenas sem alvo -> expus lastRawMm no SensorService e no comando m, confirmando 8191 (sem alvo) em vez de falha de I2C, e reflash para validar no funcionamento real

**Arquivos:**
- `firmware/irobot/DashboardService.cpp`
- `firmware/irobot/SensorService.cpp`
- `firmware/irobot/SensorService.h`
- `firmware/irobot/irobot.ino`

### 2026-11-15 20:36 - feat(irobot): autonomo continuo com deteccao de enroscamento e alerta no app

**Area:** irobot

**Feito:**
- AutoNavigator deixou de parar apos 3 giros: agora roda ate o usuario desligar ou ate acumular 100 obstaculos seguidos sem completar 2 s de avanco livre (AUTO_STUCK_ATTEMPTS e AUTO_FREE_FORWARD_MS no HardwareConfig)
- ao enroscar desliga o modo, zera o PWM e publica stuck e stuckSeq em /api/auto e /api/sensors
- painel ganhou banner de alerta com vibracao e bipe e polling de 1 s
- /teste tambem alerta
- comandos seriais continuam a e m e o log mostra a contagem de obstaculos

**Dificuldades e solucoes:**
- Nenhuma porta COM presente nas duas tentativas de gravacao -> firmware compilado (1.074.447 bytes, 81% flash) e commitado, restando gravar com tools/flash.ps1 -Sketch irobot assim que o ESP32 for reconectado
- notificacao nativa do browser exige HTTPS e no AP ela so funciona ate onde o navegador permite, entao o alerta principal ficou sendo o banner com vibracao e som

**Arquivos:**
- `firmware/irobot/AutoNavigator.cpp`
- `firmware/irobot/AutoNavigator.h`
- `firmware/irobot/DashboardService.cpp`
- `firmware/irobot/HardwareConfig.h`
- `firmware/irobot/WebAssets.h`
- `firmware/irobot/web/css/style.css`
- `firmware/irobot/web/index.html`
- `firmware/irobot/web/js/app.js`
- `firmware/irobot/web/teste.html`

### 2026-11-15 19:35 - feat(irobot): sensores AHT20/BMP280/VL53L0X e modo autonomo

**Area:** irobot

**Feito:**
- SensorService le AHT20 + BMP280 (temp/umidade/pressao) e VL53L0X (distancia) no I2C SDA=21/SCL=22 com intervalos escalonados
- AutoNavigator implementa FSM frente/para/desvia/assenta/desiste com giros alternados a cada 650 ms e limite de 3 tentativas
- novas rotas GET /api/sensors, GET e POST /api/auto e comandos seriais a e m
- painel ganhou card Ambiente com distancia e botao Autonomo e a pagina /teste mostra linha compacta com toggle
- WebAssets.h regenerado e firmware compila em 81% de flash

**Dificuldades e solucoes:**
- arduino-cli reportou sucesso com 1.070.207 bytes
- a gravacao nao pode ser feita porque nenhuma porta COM estava presente no momento do teste -> deixei o firmware compilado e orientei reconectar o ESP32 e gravar com tools/flash.ps1 -Sketch irobot

**Arquivos:**
- `firmware/irobot/DashboardService.cpp`
- `firmware/irobot/DashboardService.h`
- `firmware/irobot/HardwareConfig.h`
- `firmware/irobot/WebAssets.h`
- `firmware/irobot/irobot.ino`
- `firmware/irobot/web/css/style.css`
- `firmware/irobot/web/index.html`
- `firmware/irobot/web/js/app.js`
- `firmware/irobot/web/teste.html`
- `firmware/irobot/AutoNavigator.cpp`
- `firmware/irobot/AutoNavigator.h`
- `firmware/irobot/SensorService.cpp`
- `firmware/irobot/SensorService.h`

### 2026-11-15 19:08 - docs(irobot): registra o Prototipo-67 e o teste com duas baterias 3.7V

**Area:** irobot

**Feito:**
- README ganha secao Prototipos com o Prototipo-67
- teste com 2 baterias 3.7V em serie (7.4V) alimentou tudo, ficou um pouco fraco mas validou todas as funcionalidades

**Dificuldades e solucoes:**
- nenhuma

**Arquivos:**
- `firmware/irobot/README.md`

### 2026-11-15 18:53 - docs(irobot): registra alimentacao do ESP32 pela saida 5V do L298N

**Area:** irobot

**Feito:**
- README documenta bateria 7-12V no L298N com jumper 5V-EN, saida 5V para VIN do ESP32 e GND comum
- aviso de nao usar USB junto
- menciona a rota /teste

**Dificuldades e solucoes:**
- nenhuma

**Arquivos:**
- `firmware/irobot/README.md`

### 2026-11-15 17:12 - feat(irobot): painel antigo de volta em / e versao simples em /teste

**Area:** irobot

**Feito:**
- raiz volta a ter joystick a esquerda e sliders de velocidade a direita
- /teste serve a pagina so com 5 botoes
- gerador embute teste_html e links antigos ganham ?v=3 contra cache

**Dificuldades e solucoes:**
- esp32 parou de responder ao esptool apos matar o monitor serial -> reset fisico na placa e nova deteccao da porta resolveram

**Arquivos:**
- `firmware/irobot/DashboardService.cpp`
- `firmware/irobot/DashboardService.h`
- `firmware/irobot/WebAssets.h`
- `firmware/irobot/tools/generate_web_assets.ps1`
- `firmware/irobot/web/css/style.css`
- `firmware/irobot/web/index.html`
- `firmware/irobot/web/js/app.js`
- `firmware/irobot/web/teste.html`

### 2026-11-15 16:50 - fix(irobot): gerador de WebAssets embutia aspas no inicio do app.js

**Area:** irobot

**Feito:**
- generate_web_assets.ps1 nao adiciona mais aspas extras apos R-raw
- JS servido voltou a ser valido e os botoes conseguem chamar a API
- WebAssets.h regerado e gravado em COM5

**Dificuldades e solucoes:**
- app.js chegava ao navegador com aspas solta no inicio (SyntaxError) e nenhum fetch saia do celular -> corrigido o delimitador do raw string e conferido byte a byte no header gerado

**Arquivos:**
- `firmware/irobot/DashboardService.cpp`
- `firmware/irobot/WebAssets.h`
- `firmware/irobot/tools/generate_web_assets.ps1`

### 2026-11-15 16:17 - feat(irobot): tela simplificada so com botoes de direcao

**Area:** irobot

**Feito:**
- index.html com 5 botoes grandes (frente, tras, esquerda, direita, parar)
- app.js reescrito mostrando erro de API na propria tela
- css novo e WebAssets.h regerado

**Dificuldades e solucoes:**
- sem acesso ao AP da placa pela maquina (Windows bloqueia netsh wlan connect sem Localizacao) -> validacao feita por compilacao, gravacao e log serial de boot

**Arquivos:**
- `firmware/irobot/WebAssets.h`
- `firmware/irobot/web/css/style.css`
- `firmware/irobot/web/index.html`
- `firmware/irobot/web/js/app.js`

### 2026-11-15 16:06 - fix(irobot): tela nao enviava comandos por ler o corpo do POST do socket

**Area:** irobot

**Feito:**
- POST usa server.arg('plain') em move e config
- o corpo ja vem bufferizado pelo WebServer e readString devolvia vazio esperando 1s por requisicao

**Dificuldades e solucoes:**
- handlers liam client().readString() depois do parser consumir o corpo, causando 400 e travando o servidor -> trocado por arg('plain') e regravado no ESP32

**Arquivos:**
- `firmware/irobot/DashboardService.cpp`

### 2026-11-15 15:58 - fix(tooling): flash.ps1 falhava com uma unica porta serial

**Area:** tooling

**Feito:**
- ports agora e sempre array com @() e a gravacao segue sem -Port
- firmware irobot compilado e gravado com sucesso em COM8 (ESP32-D0WD-V3)

**Dificuldades e solucoes:**
- PowerShell desembrulha array de um elemento e .Count fica vazio -> Get-SerialPorts embrulhado em @() e campo Count validado

**Arquivos:**
- `tools/flash.ps1`

### 2026-11-15 15:53 - docs(devlog): corrige entrada quebrada pelo separador de itens

**Area:** docs, raiz

**Feito:**
- entrada do commit anterior reorganizada em bullets
- AGENTS.md avisa para nao usar ponto e virgula dentro do texto de um item

**Dificuldades e solucoes:**
- o proprio texto do item continha o separador e foi dividido ao meio -> texto reescrito sem o caractere

**Arquivos:**
- `AGENTS.md`
- `docs/DEVLOG.md`

### 2026-11-15 15:52 - fix(tooling): itens de Summary/Difficulty separados por ';' no save.ps1

**Area:** docs, raiz, tooling

**Feito:**
- save.ps1 divide -Summary e -Difficulty pelo ponto e virgula (powershell -File nao converte virgula em lista)
- entrada anterior do DEVLOG reorganizada em bullets
- AGENTS.md atualizado com o separador

**Dificuldades e solucoes:**
- powershell.exe -File entregou "a","b" como uma unica string e o DEVLOG saiu corrido -> split pelo separador e entrada corrigida

**Arquivos:**
- `AGENTS.md`
- `docs/DEVLOG.md`
- `tools/save.ps1`

### 2026-11-15 15:51 - feat(irobot): chassi 2 rodas + caster, deteccao automatica de placa/COM e DEVLOG automatico

**Area:** irobot, windfollowblind, tooling, docs

**Feito:**
- chassi documentado como 2 motores (1 roda cada) + roda caster livre
- novo `tools/flash.ps1`: detecta COM na hora, confere chip com esptool e grava
- novo `tools/save.ps1` + `docs/DEVLOG.md`: commit, push e diário automáticos
- `AGENTS.md`: fluxo de início de sessão e regras de porta/commit
- corrige `windfollowblind.ino` (não compilava) e typos de mDNS do blind
- remove COM fixa dos READMEs/documentação; README raiz ganha seção irobot

**Dificuldades e soluções:**
- arduino-cli marca clone CH340 como Unknown -> chip identificado via esptool flash-id
- `windflollowblind.ino` não batia com a pasta (main file missing) -> `git mv` preservando histórico
- pasta de build com `%TEMP%` literal dentro do repo -> removida; scripts usam TEMP
- PowerShell 5.1 lê .ps1 sem BOM como ANSI -> scripts mantidos em ASCII puro

**Arquivos:**
- `README.md`
- `docs/WIND_FOLLOW_BLIND_DOCUMENTACAO.md`
- `firmware/irobot/DashboardService.cpp`
- `firmware/irobot/HardwareConfig.h`
- `firmware/irobot/README.md`
- `firmware/windfollowblind/DashboardServiceBlind.cpp`
- `firmware/windfollowblind/windflollowblind.ino -> firmware/windfollowblind/windfollowblind.ino`
- `AGENTS.md`
- `docs/DEVLOG.md`
- `tools/flash.ps1`
- `tools/save.ps1`
