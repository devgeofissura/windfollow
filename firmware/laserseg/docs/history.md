# Histórico compacto — laserseg

> **Para que serve:** memória de sessão. Um modelo de médio contexto deve conseguir ler
> este arquivo inteiro. Detalhes ficam em `README.md`, `ESTADO.md`, `PROTOCOLO_UDP.md` e
> nos documentos de `docs/example01/` e `docs/example02/`. Atualize no fim de cada commit,
> com entradas curtas.

## 0. Regra de ouro

1. T-SIMCAM faz **visão**; ESP32-S3 separada faz **motores + luz**. Não inverter.
2. `docs/example01/` é protótipo compilável, **não** firmware validado em bancada.
3. Luz desligada no boot, no reset, na perda de link e no watchdog: hoje por
   contato NA do rele serial do laser; antigo `LED_EN` esta SUPERADO.
4. Motores param com fin de curso NF, comando inválido ou link > 1200 ms.
5. Sensor, pinout, chip, flash e MAC não confirmados não viram fato. Diga "não confirmado".
6. Porta COM nunca fixa; detectar sempre antes de gravar.
7. **Nunca sobrescrever nem apagar código sem backup prévio.** Antes de qualquer
   substituição/remoção: ler o arquivo, guardar cópia (`*.bak` fora do repo ou
   commit anterior), mostrar o diff e só então aplicar. Nada de `write` cego.
8. **Migration de banco com cuidado.** Antes de aplicar: inspecionar dados atuais,
   fazer backup verificável e definir rollback. `DROP`, `TRUNCATE`, `DELETE` e
   mudanças destrutivas só com confirmação explícita do usuário e backup testado.
9. **Python sempre LTS/estável**, nunca a versão mais recente. Interpretador atual:
   3.14.10 em `%LOCALAPPDATA%\Programs\Python\Python314\`.

## 1. Commits e marcos

Repositório: `git@github.com:devgeofissura/windfollow.git`, branch `main`.

| Hash / mensagem | Marco |
|---|---|
| `ced0004` | `feat(msa): lista redes wifi no modo cliente` — estado inicial da sessão |
| `1dab9d9` | `fix(msa): ajusta interface mobile first` |
| `7a4ab17` | `feat(msa): adiciona OLED e interface responsiva` |
| `63063c7` | `feat(msa): cria firmware de sensores ambientais` |
| `6094d3c` | `fix(irobot): corrige ordem dos botões do D-pad` |
| `e243724` | `docs: consolida licoes aprendidas do iRobot e Windfollow` — ver `docs/LICOES_APRENDIDAS.md` na raiz |
| `f3c3a1f` | `feat(irobot): layout Home responsivo em retangulo perfeito` |
| `a666c98` | `feat(irobot): ajusta UI Home, icones PWA e preview local` |
| `250910c` | `docs: README profissional com todos os projetos...` |
| `73d48a9` | `feat(irobot): nova UI web com abas Home/DL/Config, PWA...` |
| `a1ca948` | `docs(laserseg): fundacao com skills e agentes` — README, ESTADO, protocolo, AGENTS, 13 skills, 3 agentes, MCPs e este histórico |
| `6324067` | `docs(laserseg): regra de backup e migrations seguras` — regras 7 e 8 |
| `e832bd5` | `feat(laserseg): especialistas esp32 e lilygo` — skill local `espressif-lilygo`, 39 skills `esp-*` globais, atribuições e documentação |
| `26f0547` | `docs(laserseg): path do python aponta para a versao lts` |
| `—` | `feat(infra): mqtt local, websocket e quick tunnel` — Mosquitto 2.1.4 portable, scripts `tools/mqtt/`, `MQTT.md`, perfil `laserseg` e `arduino-cli` 1.6.0/core 3.3.11 |
| `ffff575` | `test(laserseg): parser udp validado e probe serial versionado` — `tools/protocol_ref.py`, 84 testes, contrato v0.1 e revisão independente aplicada |
| `7f54c65` | `chore: remove pycache versionado do parser` |
| `44cf96d` | `feat(laserseg): parser UDP C++ com autoteste e telemetria` — `controller/`, 55 vetores gerados da referência Python, perfil repontado e 6 condições de integração |
| `f580b44` | `fix(laserseg): telemetria em maiusculas validada na placa` — `AUTOTEST PASS` 76/76; bug de `none`/`range`/`link` minusculos só apareceu no dispositivo |
| `cbe2f69` | `feat(laserseg): no DM556D serial-only com nucleo compartilhado` — `controller_dm556d/` reusa parser e constantes validados |
| `99f7581` | `docs(laserseg): nota de verificacao de hardware T1` |
| `440ed12` | `docs(laserseg): versiona prototipos e spec wifi provisioning` — `example/` → `example01/` com 18 arquivos byte-identicos, `example02/`, spec AP → STA aprovada e 92 testes host verdes |
| `a69cc57` | `docs(laserseg): spec do simulador web V1` — gate spec/aprovacao/completo; premissas, limites e criterios de aceite |
| `—` | `feat(laserseg): simulador web V1` — `simulator/` com Painel/Wi-Fi/Protocolo, 10 testes de contrato e 102 testes host verdes |
| `—` | `docs(laserseg): hardware em WireViz e aba Pinout` — 3 YAMLs validados, pinout primario da SIMCAM, divergencias preservadas |
| `—` | `feat(laserseg): saida de luz laser via rele serial` — conversor 3,3→5 V, fontes separadas, contato NA + emergencia + watchdog; aba Pinout somente leitura |

> Hash exato do commit atual: `git log -1 --oneline`.

## 2. Arquitetura e motivo

```text
[cena/alvo] → [T-SIMCAM S3_CAM_SIM V1.3] → UDP 4210 → [ESP32-S3 controladora]
                                                          ├→ driver PAN
                                                          ├→ driver TILT
                                                          └→ conversor 3,3→5 V → rele serial → LASER
```

- Separação escolhida porque a S3_CAM_SIM V1.3 tem GPIOs de borda com funções
  reservadas (flash, modem, LED mPCIe segundo relato do issue #33) e apenas
  quatro GPIOs livres segundo o pinout primário do usuário. Não depender dela
  para motores/iluminação evita conflito elétrico; a divergência entre as duas
  fontes permanece registrada em `docs/wireviz/no-visao-s3-camsim.yaml`.
- A placa continua candidata à **visão**; o que não se pode fazer é assumir pinos livres
  sem esquema da revisão.
- Detecção por diferença entre quadros: não reconhece pessoa/animal/objeto e reage a
  mudança de iluminação. Sem identificação biométrica.
- Não transmitir vídeo para outro computador; só erro/posição e comandos.

## 3. Protocolo UDP (contrato mínimo)

| Mensagem | Formato | Efeito |
|---|---|---|
| `TRACK,0\|1` | liga/desliga seguimento | `0` para eixos |
| `LIGHT,0\|1` | liga/desliga foco | `0` no boot e no timeout |
| `MANUAL,pan\|tilt,-1\|1` | passo manual | 80 passos por comando |
| `T,seq,x,y,valid` | amostra | `x/y` em -1000..1000, `valid` 0/1 |

- AP: câmera `192.168.4.1`, controladora `192.168.4.2`, porta `4210`, sem autenticação.
- `DEAD_BAND = 55`; `MAX_TRAVEL = STEPS_PER_REV / 6` (≈ ±60°); `LINK_TIMEOUT_MS = 1200`.
- `TRACK_GAIN`: 0.75 no protótipo DRV8834, 0.55 no ULN2003. Não copiar às cegas.
- Sentido de `x/y` ainda não confirmado em ensaio: não inverter "por intuição".
- Contrato v0.1 fechado: faixa/sequência/idade em **todo** comando, rejeição de
  repetidos e telemetria `ACK,<seq>,<state>,<fault>,<light>` (envio de rede pendente).
- Parser C++ em `controller/` com vetores **gerados** de `protocol_ref.py`; nunca
  manter tabela duplicada à mão. Telemetria usa `NONE/RANGE/LINK` em **maiúsculas**
  (mapa próprio; `reasonName()` é minúsculo). Autoteste na placa: 76/76 PASS.
  Antes de GPIO: `fault()` ⇒ parada+luz OFF,
  `poll()` antes dos motores, sem `now_ms=-1` em campo, corrigir rollover do
  `millis()`. Ver `PROTOCOLO_UDP.md`.

## 4. Hardware e segurança

- Iluminador = **laser** via módulo relé serial RoboCore (decisão de 2026-11-20):
  Clock/Data 5 V exige conversor 3,3→5 V, bobinas 12 V em fonte separada, contato
  NA + chave física de emergência + watchdog. Classe/potência e anteparo ainda
  **NAO CONFIRMADOS**; nunca apontar a pessoas, animais, veículos ou aeronaves.
- GPIOs Clock/Data do relé ainda não escolhidos; nunca alimentar laser ou bobinas
  diretamente deles. Laser OFF no boot/reset, antes de qualquer `loop`.
- Fins de curso NF com `INPUT_PULLUP`: HIGH bloqueia o sentido; fio rompido também bloqueia.
  Sem `INPUT_PULLDOWN`, sem homing automático; eixos começam centralizados manualmente.
- Precedência: parada física > fins de curso > watchdog > mensagem inválida > zona morta.
- Alimentação de motor separada/capaz, GND comum, decoupling; não alimentar motor pelo
  regulador da placa. Driver compatível com o motor real (bipolar+DRV8834 ou 28BYJ-48+ULN2003).
- **Incógnitas:** sensor OV2640 vs OV5640, revisão do pinout, placa controladora exata,
  driver/motor, corrente do foco.

## 5. Build/flash — bloqueios atuais

- `arduino-cli` 1.6.0 fora do PATH; `tools/flash.ps1` tem fallback para o caminho absoluto.
- Porta detectada em 2026-11-19: `COM4` (`303A:1001`, USB nativo), ESP32-S3 16 MB,
  MAC `dc:da:0c:28:25:30`. Nunca fixar a porta; detectar de novo antes de gravar.
- `tools/flash.ps1` aceita `-Sketch laserseg` com FQBN própria e build em `%TEMP%`;
  perfil aponta para `controller/controller.ino` (probe `serial_probe/` é fallback).
- FQBN dos protótipos: `esp32:esp32:esp32s3:FlashSize=16M,PartitionScheme=app3M_fat9M_16MB,PSRAM=opi,USBMode=hwcdc,CDCOnBoot=cdc,CPUFreq=240`.
  Perfil `windfollow` da raiz usa `PartitionScheme=huge_app`. **Não misturar** sem validar
  com `arduino-cli board details`.
- Build sempre em `$env:TEMP`; nunca criar pasta de build no repositório.
- Fatos oficiais LILYGO (repo MIT `Xinyuan-LilyGO/T-SimCam`): Arduino-ESP32 3.3.11,
  serial 115200, `BOARD_POWER_ON_PIN=1` HIGH obrigatório, PSRAM OPI, partição
  `app3M_fat9M_16MB`, V1.3 = mic I2S (BCLK 41/WS 42), V1.4 = PDM (CLK 42).
  Sensor da câmera **ainda não confirmado** pelo repo; não afirmar OV2640/OV5640.

## 6. OpenCode V2 — aprendizados

- Formatos: `.opencode/skills/<id>/SKILL.md`, `.opencode/agents/<nome>.md`,
  config em `.opencode/opencode.jsonc`.
- Config usada: `$schema`, `default_agent`, `instructions`, `mcp.servers`.
  Servidor local usa `type: local` + `command[]`; remoto usa `url`; segredo vai em
  `{env:NOME}` e servidor desligado usa `disabled: true`.
- Validar com `opencode debug config` e `opencode debug agents`. `opencode mcp list`
  reportou "No MCP servers configured" mesmo com a config carregada (limitação do CLI
  v2.3.2 para servers de projeto); usar `/mcps` na TUI para conectar/autenticar.
- `npx.ps1` é bloqueado pela ExecutionPolicy do PowerShell. Por isso os skills do GitHub
  foram instalados por clone manual e cópia dos diretórios, preservando
  `../../references/...`.
- Instalados de `addyosmani/agent-skills` (MIT): 9 skills, 4 checklists em
  `.opencode/references/`, 3 personas em `.opencode/vendor-agents/` (não são agentes ativos).
- Skills próprios: `laserseg-context`, `espressif-lilygo`, `esp32-build-workflow`,
  `udp-protocol`, `hardware-safety`. Agentes locais: `laserseg-dev`,
  `laserseg-reviewer`, `hardware-auditor`.
- Especialistas instalados globalmente: 39 skills `esp-*` de
  `grantwilk/espressif-skills` (MIT) em `~/.config/opencode/skills/embedded/espressif/`
  — índice em `esp-toolchain-overview`. Kit global: 150 skills, 103 agentes.
  `espressif/skills` oficial (Apache-2.0) estava vazio em 2026-11-19; não instalado.
- Agentes úteis: `embedded-engineer` (firmware), `vision-engineer` (visão),
  `robotics-engineer` (motores), `security-engineer`, `code-reviewer`.
- MCP `context7` remoto configurado; `github` local preparado e desabilitado até existir
  `GITHUB_PERSONAL_ACCESS_TOKEN`.
- `espressif-lilygo` agora é **local e global** (mesmo SHA-256); ao editar, sincronize as duas.
- Python 3.14.10 LTS + `PyYAML 6.0.3` + `pyserial 3.5`; 3.15.2 e o stub da Store descartados.
- Frontmatter auditado com parser YAML real: **249 skills, 0 erros**. "Procurar `---`"
  não é validação (CRLF/BOM geravam falso positivo no validador Node).

## 6.1 Infra da estação (fora do firmware)

- `arduino-cli` 1.6.0 em `%LOCALAPPDATA%\Programs\arduino-cli`; core
  `esp32:esp32@3.3.11` (versão validada pela LILYGO, não a 3.3.12).
- Perfil `laserseg` em `tools/flash.ps1` (FQBN S3, `app3M_fat9M_16MB`, OPI
  PSRAM, build em `%TEMP%\laserseg-build`); `Get-ArduinoCli` tem fallback
  para o caminho absoluto.
- MQTT: Mosquitto 2.1.4 portable em `%LOCALAPPDATA%\Programs\mosquitto`,
  TCP `127.0.0.1:1883` e WebSocket `127.0.0.1:9001`, auth obrigatória
  (`laserseg`). Operação em `tools/mqtt/`; detalhes em `MQTT.md`.
- Túnel: `cloudflared` 2026.11.1, Quick Tunnel `wss://<subdominio>.trycloudflare.com/mqtt`.
  Validado com `paho-mqtt` (TCP e WSS). Quick Tunnel só cobre HTTP/HTTPS;
  TCP puro (Postgres 5432) exige Named Tunnel + WARP.
- Credenciais MQTT em `%USERPROFILE%\.config\opencode\secrets\mqtt-local.txt`
  (ACL restrita). Nunca no repositório.
- Postgres: `pg_hba.conf` liberado para faixas RFC1918 (LAN), sem acesso
  externo; firewall ainda pendente de execução elevada.
- Probe de serial (sem GPIO) versionado em `serial_probe/serial_probe.ino`, compilado e
  gravado com a FQBN `laserseg`; a placa respondeu `laserseg probe: alive`.
  O perfil `tools/flash.ps1 -Sketch laserseg` aponta para ele até existir produção.
- Parser UDP: referência em `tools/protocol_ref.py` e **84 testes** em
  `tests/test_protocol.py` (`tests/run.ps1` usa o Python LTS absoluto). Contrato
  v0.1 em `PROTOCOLO_UDP.md`: tamanho/codificação → formato → faixa →
  sequência → idade; a idade vale para QUALQUER comando; somente datagrama
  aceito atualiza `lastPacket`; `TRACK,1` reinicia a janela de sequência.
  `axes_stopped` explícito no boot, no `TRACK,0`, em amostra inválida e no
  watchdog — o firmware deve parar AMBOS os eixos e desligar a luz.
  Protótipos em `docs/example01/` não cumprem isso (`toInt()` aceita lixo).
- Revisão `laserseg-reviewer` (somente leitura) apontou: idade só em `T`,
  watchdog sem parada de eixos, perfil apontando para sketch inexistente e
  três documentos defasados. Tudo corrigido no mesmo bloco; `Stop-Process`
  global em `tools/mqtt/` ficou como pendência de baixa prioridade.
- `arduino-cli` exige que o `.ino` principal tenha o mesmo nome da pasta: o probe
  precisou ser renomeado de `probe/serial_probe.ino` para
  `serial_probe/serial_probe.ino` (o perfil `Ino` sozinho não basta).

## 7. Decisões fixadas

1. T-SIMCAM como nó de visão; não acionar motores/iluminador diretamente por ela.
2. Enviar só posição/erro e comandos por UDP; sem vídeo.
3. `AccelStepper` na controladora, fins de curso NF e timeout de link.
4. Protótipos são fonte de ideias, não de verdade elétrica.
5. Laser via relé serial desde 2026-11-20, com conversor 3,3→5 V, fonte 12 V
   separada e contato NA + emergência + watchdog; antigo LED+MOSFET está SUPERADO.
6. Mudança de firmware atualiza `ESTADO.md`, `PROTOCOLO_UDP.md` e `DEVLOG.md` no mesmo commit.

## 8. Pendências em ordem

1. Re-detectar a placa antes de cada gravação (a porta pode mudar).
2. Confirmar sensor e esquema/pinout da S3_CAM_SIM V1.3.
3. Escolher controladora, driver e motores por torque/corrente reais.
4. Implementar o parser C++ espelhando `protocol_ref.py` e rodar os mesmos casos na placa.
5. Prever telemetria `ack_seq`, `state` e `fault` antes de operação supervisionada.
6. Ensaio de bancada sem carga, foco desligado, limites conservadores.
7. Migrar o Quick Tunnel para Named Tunnel antes de sair da bancada.
8. Firewall (1883/5432) depende de execução elevada: `tools/mqtt/firewall.ps1`.

## 9. Comandos úteis

```powershell
# raiz do repositório — detectar sem perguntar
powershell -ExecutionPolicy Bypass -File tools/flash.ps1 -DetectOnly

# validação OpenCode (dentro de firmware/laserseg)
& "$env:APPDATA\npm\opencode.cmd" debug config
& "$env:APPDATA\npm\opencode.cmd" debug agents

# auditoria real de frontmatter (Python LTS + PyYAML)
& "$env:LOCALAPPDATA\Programs\Python\Python314\python.exe" `
  "$env:TEMP\opencode\yaml-check\validate.py"

# fechar tarefa (raiz) — gera DEVLOG, commit e push
powershell -ExecutionPolicy Bypass -File tools/save.ps1 `
  -Message "docs(laserseg): <resumo>" `
  -Summary "<item 1>; <item 2>" `
  -Difficulty "<problema> -> <solucao>"
```

## 10. Mapa rápido de arquivos

| Arquivo | Papel |
|---|---|
| `firmware/laserseg/README.md` | visão geral, decisões, como usar OpenCode |
| `firmware/laserseg/AGENTS.md` | regras e mapeamento intenção → skill |
| `firmware/laserseg/docs/ESTADO.md` | status e pendências detalhadas |
| `firmware/laserseg/docs/PROTOCOLO_UDP.md` | contrato completo das mensagens |
| `firmware/laserseg/docs/foco-luz-seguidor-001` | ligação revisada e memorial |
| `firmware/laserseg/docs/example01/` | sketches, CAD e referências (protótipos) |
| `firmware/laserseg/docs/example02/simcam_tracker/` | protótipos organizados por nó |
| `firmware/laserseg/.opencode/` | config, skills, agentes, MCPs, atribuição MIT |
| `docs/LICOES_APRENDIDAS.md` (raiz) | lições dos projetos iRobot/Windfollow |

## 11. Como manter

- Uma linha nova por commit na tabela do item 1; nunca apague linhas antigas.
- Se passar de ~200 linhas, resuma as seções 2–6 e mova o excedente para `ESTADO.md`.
- Não cole segredo, token, MAC "confirmada" ou porta COM fixa aqui.
