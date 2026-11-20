# laserseg — seguidor de luz com ESP32

> **Estado atual (2026-11-20):** fundação do subprojeto + simulador web V1. Documentação, regras, skills, agentes e MCPs criados antes do firmware de produção; `simulator/` abre no navegador sem placa. Os sketches em `docs/example01/` e `docs/example02/` são **protótipos de referência**, não código final.

## Escopo

Seguir visualmente um alvo em uma montagem pan-tilt e manter um foco de luz apontado para a cena de teste. Em 2026-11-20 o iluminador foi definido como **laser** acionado por **módulo relé serial RoboCore** (2 pinos Clock/Data em 5 V, bobinas 12 V, cascata até 10 módulos). A controladora é 3,3 V, então o **conversor de nível é obrigatório**. Intertravamento: contato NA (desenergizado = laser OFF), chave de emergência e watchdog. Classe/potência do laser, GPIOs Clock/Data e fontes seguem **não confirmados** — nenhum GPIO é habilitado antes da auditoria (`LASERSEG_ENABLE_GPIO = 0`). Nunca apontar para pessoas, animais, veículos ou aeronaves.

A arquitetura adotada separa responsabilidades:

```text
[Cena / alvo]
      │ luz
      ▼
[T-SIMCAM S3_CAM_SIM V1.3]  captura + rastreamento
      │ UDP 4210 (posição/erro + comandos)
      ▼
[ESP32-S3 controladora]  STEP/DIR motores + fins de curso + laser
      ├──► driver PAN  ──► motor PAN
      ├──► driver TILT ──► motor TILT
      └──► conversor 3,3→5 V ──► módulo relé serial ──► LASER
```

Motivo da separação: a S3_CAM_SIM V1.3 tem GPIOs de borda com funções já reservadas (flash, modem, LED mPCIe). Não depender dela para acionar motores, iluminador e fins de curso evita conflito elétrico. A placa continua plenamente candidata à **visão**; o que não se deve é assumir pinos livres sem esquema/revisão confirmada. O pinout detalhado enviado pelo usuário em 2026-11-20 é a fonte primária, em [`docs/wireviz/no-visao-s3-camsim.yaml`](docs/wireviz/no-visao-s3-camsim.yaml); divergências com a tabela oficial e o issue #33 não foram resolvidas por suposição.

## Decisões fixadas por enquanto

| Tema | Decisão |
|---|---|
| Visão | T-SIMCAM faz captura e localização do alvo; sem identificação biométrica |
| Controle | Controladora ESP32-S3 separada com `AccelStepper` |
| Link | UDP na porta `4210`, somente em rede confiável, sem autenticação no protótipo |
| Foco | Laser via módulo relé serial RoboCore; conversor 3,3→5 V obrigatório, GPIOs Clock/Data a definir |
| Segurança | Fins de curso NF, `INPUT_PULLUP`, contato NA do relé + chave de emergência + watchdog de 1200 ms; laser OFF no boot/reset/perda de link |
| Curso | Limite lógico inicial de ±60° em torno do centro manual |
| Sensor | **Não confirmado** — pode ser OV2640 IR-CUT ou OV5640 IR-CUT |
| Firmware | Exemplos servem de base; ainda será criada a estrutura de produção |

Detalhes em [`docs/PROTOCOLO_UDP.md`](docs/PROTOCOLO_UDP.md), [`docs/ESTADO.md`](docs/ESTADO.md), [`docs/base-de-conhecimento-rastreamento-com-s3-cam-sim.md`](docs/base-de-conhecimento-rastreamento-com-s3-cam-sim.md) e nos artefatos versionáveis de [`docs/wireviz/`](docs/wireviz/00-LEIA-ME.md) (esquemático, diagrama de ligação e pinouts).

## Estrutura

```text
firmware/laserseg/
├── AGENTS.md                         regras específicas do subprojeto
├── README.md                         este arquivo
├── simulator/                        simulador web V1 (duplo clique, sem servidor)
├── spec/                             specs aprovadas (UDP, DM556D, Wi-Fi, simulador)
├── tasks/                            planos e tarefas (inclui o simulador)
├── docs/
│   ├── ESTADO.md                     status, pendências e próximos passos
│   ├── history.md                    memória compacta do desenvolvimento
│   ├── PROTOCOLO_UDP.md              contrato de mensagens entre os nós
│   ├── base-de-conhecimento-...md    escopo e evidências
│   ├── foco-luz-seguidor-001         memorial de ligação revisada
│   ├── wireviz/                     esquemático, ligação e pinouts (YAML)
│   ├── example01/                   protótipos, CAD, zips e referências
│   ├── example02/simcam_tracker/    protótipos organizados por nó
└── .opencode/
    ├── opencode.jsonc                config do projeto (agente, instruções, MCP)
    ├── agents/                       agentes especializados
    ├── skills/                       skills próprios + skills instalados
    ├── references/                   checklists dos skills instalados
    ├── vendor-agents/                personas instaladas (somente referência)
    ├── THIRD_PARTY.md                atribuição e licença
    └── THIRD_PARTY_LICENSES/         licença integral do upstream
```

Pastas de firmware ainda serão criadas: `camera_node/`, `controller_drv8834/` e `controller_uln2003/`.

## Simulador web (V1)

Abra `simulator/index.html` com duplo clique: funciona em `file://`, sem servidor,
build ou placa. Quatro abas — **Painel** (webcam/alvo sintético, `SAFE/TRACK/MANUAL`,
`x/y/seq`, watchdog e log `T`/`ACK`), **Wi-Fi** (AP `laserseg`/`123456`, varredura
simulada, senha da rede **somente em memória**, fallback "AP e para"),
**Protocolo** (constantes e formatos UDP, somente leitura) e **Pinout** (mapa da
S3_CAM_SIM e ressalva do laser via relé serial, somente leitura). A aba Pinout
não altera a FSM `SAFE/TRACK/MANUAL` nem o comando `LIGHT`, e não aciona hardware.

```powershell
powershell -ExecutionPolicy Bypass -File firmware/laserseg/tests/run.ps1
node --check firmware/laserseg/simulator/js/app.js
```

A senha do AP é pública por decisão aprovada; nenhuma credencial de rede é
gravada. O simulador valida a experiência e o contrato, **não** o hardware.
Detalhes em [`simulator/README.md`](simulator/README.md),
[`spec/SPEC-simulador-web.md`](spec/SPEC-simulador-web.md) e
[`tasks/plan-simulador-web.md`](tasks/plan-simulador-web.md).

## Usando OpenCode neste subprojeto

Abra o OpenCode com `firmware/laserseg` como diretório de trabalho. O arquivo `.opencode/opencode.jsonc` define `laserseg-dev` como agente principal, carrega as instruções do projeto e registra os MCPs.

Skills próprios:

- `laserseg-context` — carrega escopo, decisões e pendências antes de qualquer mudança.
- `espressif-lilygo` — toolchain Espressif, pinagem oficial T-SimCam V1.3/V1.4,
  core/partição e regras de detecção antes de gravar.
- `esp32-build-workflow` — compilação, FQBN, detecção de porta e regras de gravação.
- `udp-protocol` — contrato de mensagens e comportamento da controladora.
- `hardware-safety` — GPIOs, alimentação, motores, fins de curso e foco de luz.

Skills instalados do [addyosmani/agent-skills](https://github.com/addyosmani/agent-skills)
(MIT): `spec-driven-development`, `planning-and-task-breakdown`,
`incremental-implementation`, `test-driven-development`, `code-review-and-quality`,
`security-and-hardening`, `debugging-and-error-recovery`, `documentation-and-adrs` e
`constraint-driven-development`. Os checklists compartilhados foram preservados em
`.opencode/references/`. Atribuição completa em [`.opencode/THIRD_PARTY.md`](.opencode/THIRD_PARTY.md).

A skill `espressif-lilygo` existe **local e globalmente** (mesmo conteúdo): local em
`.opencode/skills/espressif-lilygo/` e global em
`~/.config/opencode/skills/embedded/espressif-lilygo/`. Ao atualizar uma, atualize a outra.

Especialistas Espressif instalados **globalmente** (39 skills MIT de
[`grantwilk/espressif-skills`](https://github.com/grantwilk/espressif-skills)):
`esp-toolchain-overview`, `esp-detect-device`, `esp-chip-info`, `esp-idf-build`,
`esp-flash-project`, `esp-troubleshoot`, `esp-decode-crash`, `esp-efuse` e outros.
Também estão disponíveis `esp32-development`, `arduino-development`, `rtos-basics`,
`mqtt-protocol` e `sensor-integration` do kit global. Veja
[`.opencode/THIRD_PARTY.md`](.opencode/THIRD_PARTY.md) para atribuição e licenças.

Python de apoio: **3.14.10 LTS** em `%LOCALAPPDATA%\Programs\Python\Python314\`,
com `PyYAML 6.0.3` e `pyserial 3.5`. Use sempre LTS, nunca a versão mais recente.
Todas as 249 `SKILL.md` passaram na auditoria de frontmatter com parser YAML real.

Agentes disponíveis:

- Globais: `embedded-engineer` (firmware/drivers), `vision-engineer` (visão),
  `robotics-engineer` (motores), `security-engineer` e `code-reviewer`.
- `laserseg-dev` — agente principal de implementação e documentação.
- `laserseg-reviewer` — revisão somente leitura de código, protocolo e documentação.
- `hardware-auditor` — auditoria somente leitura de hardware e segurança.

MCPs configurados:

- `context7` — consulta de documentação de bibliotecas/frameworks; pode pedir login na primeira conexão.
- `github` — preparado, mas **desabilitado**; habilitar após definir `GITHUB_PERSONAL_ACCESS_TOKEN`.

Se o MCP pedir login, use `/mcps` no OpenCode. Não coloque tokens dentro dos arquivos do repositório.

## Comandos úteis

Detectar placa/porta antes de qualquer gravação (raiz do repositório):

```powershell
powershell -ExecutionPolicy Bypass -File tools/flash.ps1 -DetectOnly
```

> O `arduino-cli` 1.6.0 está fora do PATH, mas `tools/flash.ps1` usa o caminho absoluto. A detecção de 2026-11-19 encontrou uma placa ESP32-S3 nativa (`303A:1001`), 16 MB, MAC `dc:da:0c:28:25:30`. Isso é um resultado daquele momento, não uma porta fixa: rode `-DetectOnly` de novo antes de gravar.

Registrar e publicar qualquer mudança concluída:

```powershell
powershell -ExecutionPolicy Bypass -File tools/save.ps1 `
  -Message "docs(laserseg): fundação do subprojeto" `
  -Summary "..." `
  -Difficulty "..."
```

Nunca fixar porta COM no código ou na documentação como se fosse permanente.

## Referências

- [Base de conhecimento](docs/base-de-conhecimento-rastreamento-com-s3-cam-sim.md)
- [Ligação revisada — S3_CAM_SIM V1.3 + pan-tilt + laser via relé](docs/foco-luz-seguidor-001)
- [Documentação de hardware em WireViz](docs/wireviz/00-LEIA-ME.md)
- [SIMCAM Tracker — Arduino CLI + pan-tilt](docs/example01/SIMCAM%20Tracker%20—%20Arduino%20CLI%20%2B%20pan-tilt.md)
- [Ligações e ensaio](docs/example01/Ligações%20e%20ensaio.md)
- [AGENTS.md do repositório](../../AGENTS.md)
