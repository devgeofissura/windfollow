# Histórico compacto — laserseg

> **Para que serve:** memória de sessão. Um modelo de médio contexto deve conseguir ler
> este arquivo inteiro. Detalhes ficam em `README.md`, `ESTADO.md`, `PROTOCOLO_UDP.md` e
> nos documentos de `docs/example/`. Atualize no fim de cada commit, com entradas curtas.

## 0. Regra de ouro

1. T-SIMCAM faz **visão**; ESP32-S3 separada faz **motores + luz**. Não inverter.
2. `docs/example/` é protótipo compilável, **não** firmware validado em bancada.
3. LED desligado no boot, no reset, na perda de link e no watchdog.
4. Motores param com fin de curso NF, comando inválido ou link > 1200 ms.
5. Sensor, pinout, chip, flash e MAC não confirmados não viram fato. Diga "não confirmado".
6. Porta COM nunca fixa; detectar sempre antes de gravar.
7. **Nunca sobrescrever nem apagar código sem backup prévio.** Antes de qualquer
   substituição/remoção: ler o arquivo, guardar cópia (`*.bak` fora do repo ou
   commit anterior), mostrar o diff e só então aplicar. Nada de `write` cego.
8. **Migration de banco com cuidado.** Antes de aplicar: inspecionar dados atuais,
   fazer backup verificável e definir rollback. `DROP`, `TRUNCATE`, `DELETE` e
   mudanças destrutivas só com confirmação explícita do usuário e backup testado.

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
| **commit atual** | `feat(laserseg): especialistas esp32 e lilygo` — skill local `espressif-lilygo`, 39 skills `esp-*` globais, atribuições e documentação |

> Hash exato do commit atual: `git log -1 --oneline`.

## 2. Arquitetura e motivo

```text
[cena/alvo] → [T-SIMCAM S3_CAM_SIM V1.3] → UDP 4210 → [ESP32-S3 controladora]
                                                          ├→ driver PAN
                                                          ├→ driver TILT
                                                          └→ MOSFET → LED difuso
```

- Separação escolhida porque a S3_CAM_SIM V1.3 tem GPIOs de borda com funções reservadas
  (flash, modem, LED mPCIe). Não depender dela para motores/iluminação evita conflito elétrico.
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
- Evolução necessária: validar faixa/sequência/idade, rejeitar repetidos, e prever
  `ack_seq`, `state` e `fault` na volta.

## 4. Hardware e segurança

- Preferir **LED branco difuso**. Laser só com Classe 1/2, ≤ 1 mW, anteparo fosfo próximo,
  feixe abaixo dos olhos, nunca pessoas/animais/veículos/aeronaves.
- GPIO comanda **MOSFET/driver**, nunca o foco diretamente. Led OFF antes de qualquer `loop`.
- Fins de curso NF com `INPUT_PULLUP`: HIGH bloqueia o sentido; fio rompido também bloqueia.
  Sem `INPUT_PULLDOWN`, sem homing automático; eixos começam centralizados manualmente.
- Precedência: parada física > fins de curso > watchdog > mensagem inválida > zona morta.
- Alimentação de motor separada/capaz, GND comum, decoupling; não alimentar motor pelo
  regulador da placa. Driver compatível com o motor real (bipolar+DRV8834 ou 28BYJ-48+ULN2003).
- **Incógnitas:** sensor OV2640 vs OV5640, revisão do pinout, placa controladora exata,
  driver/motor, corrente do foco.

## 5. Build/flash — bloqueios atuais

- `arduino-cli` ausente do PATH nesta máquina. `tools/flash.ps1 -DetectOnly` falhou.
- `COM3` aparece como USB-SERIAL CH340, mas chip/flash/MAC não foram confirmados.
- `tools/flash.ps1` não aceita `-Sketch laserseg`; o ValidateSet tem irobot, msa, windfollow,
  windfollowblind. Sem perfil de build/flash para este subprojeto.
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

## 7. Decisões fixadas

1. T-SIMCAM como nó de visão; não acionar motores/iluminador diretamente por ela.
2. Enviar só posição/erro e comandos por UDP; sem vídeo.
3. `AccelStepper` na controladora, fins de curso NF e timeout de link.
4. Protótipos são fonte de ideias, não de verdade elétrica.
5. LED difuso no primeiro protótipo; laser após revisão explícita.
6. Mudança de firmware atualiza `ESTADO.md`, `PROTOCOLO_UDP.md` e `DEVLOG.md` no mesmo commit.

## 8. Pendências em ordem

1. Instalar/localizar `arduino-cli` e confirmar core ESP32.
2. Confirmar sensor e esquema/pinout da S3_CAM_SIM V1.3.
3. Escolher controladora, driver e motores por torque/corrente reais.
4. Fechar FQBN e criar perfil `laserseg` no `tools/flash.ps1`.
5. Implementar validação de parser (faixa, sequência, idade) e telemetria.
6. Testes sintéticos de UDP **antes** de ligar motores.
7. Ensaio de bancada sem carga, foco desligado, limites conservadores.

## 9. Comandos úteis

```powershell
# raiz do repositório — detectar sem perguntar
powershell -ExecutionPolicy Bypass -File tools/flash.ps1 -DetectOnly

# validação OpenCode (dentro de firmware/laserseg)
& "$env:APPDATA\npm\opencode.cmd" debug config
& "$env:APPDATA\npm\opencode.cmd" debug agents

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
| `firmware/laserseg/docs/example/` | sketches, CAD e referências (protótipos) |
| `firmware/laserseg/.opencode/` | config, skills, agentes, MCPs, atribuição MIT |
| `docs/LICOES_APRENDIDAS.md` (raiz) | lições dos projetos iRobot/Windfollow |

## 11. Como manter

- Uma linha nova por commit na tabela do item 1; nunca apague linhas antigas.
- Se passar de ~200 linhas, resuma as seções 2–6 e mova o excedente para `ESTADO.md`.
- Não cole segredo, token, MAC "confirmada" ou porta COM fixa aqui.
