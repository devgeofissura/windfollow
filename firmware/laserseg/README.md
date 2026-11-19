# laserseg — seguidor de luz com ESP32

> **Estado atual (2026-11-19):** fundação do subprojeto. Documentação, regras, skills, agentes e MCPs criados antes do firmware de produção. Os sketches em `docs/example/` são **protótipos de referência**, não código final.

## Escopo

Seguir visualmente um alvo em uma montagem pan-tilt e manter um foco de luz apontado para a cena de teste. A implementaçãoprioritária é com **LED/foco difuso**; laser só entra em discussão após revisão de segurança (Classe 1 ou 2, ≤ 1 mW, anteparo fosfo próximo, nunca pessoas/animalis/veículos/aeronaves).

A arquitetura adotada separa responsabilidades:

```text
[Cena / alvo]
      │ luz
      ▼
[T-SIMCAM S3_CAM_SIM V1.3]  captura + rastreamento
      │ UDP 4210 (posição/erro + comandos)
      ▼
[ESP32-S3 controladora]  STEP/DIR motores + fins de curso + foco LED
      ├──► driver PAN  ──► motor PAN
      ├──► driver TILT ──► motor TILT
      └──► MOSFET/driver ──► foco LED difuso
```

Motivo da separação: a S3_CAM_SIM V1.3 tem GPIOs de borda com funções já reservadas (flash, modem, LED mPCIe). Não depender dela para acionar motores, iluminador e fins de curso evita conflito elétrico. A placa continua plenamente candidata à **visão**; o que não se deve é assumir pinos livres sem esquema/revisão confirmada.

## Decisões fixadas por enquanto

| Tema | Decisão |
|---|---|
| Visão | T-SIMCAM faz captura e localização do alvo; sem identificação biométrica |
| Controle | Controladora ESP32-S3 separada com `AccelStepper` |
| Link | UDP na porta `4210`, somente em rede confiável, sem autenticação no protótipo |
| Foco | Preferir LED branco difuso; GPIO comanda apenas MOSFET/driver |
| Segurança | Fins de curso NF, `INPUT_PULLUP`, LED desligado no boot, watchdog de 1200 ms |
| Curso | Limite lógico inicial de ±60° em torno do centro manual |
| Sensor | **Não confirmado** — pode ser OV2640 IR-CUT ou OV5640 IR-CUT |
| Firmware | Exemplos servem de base; ainda será criada a estrutura de produção |

Detalhes em [`docs/PROTOCOLO_UDP.md`](docs/PROTOCOLO_UDP.md), [`docs/ESTADO.md`](docs/ESTADO.md) e [`docs/base-de-conhecimento-rastreamento-com-s3-cam-sim.md`](docs/base-de-conhecimento-rastreamento-com-s3-cam-sim.md).

## Estrutura

```text
firmware/laserseg/
├── AGENTS.md                         regras específicas do subprojeto
├── README.md                         este arquivo
├── docs/
│   ├── ESTADO.md                     status, pendências e próximos passos
│   ├── history.md                    memória compacta do desenvolvimento
│   ├── PROTOCOLO_UDP.md              contrato de mensagens entre os nós
│   ├── base-de-conhecimento-...md    escopo e evidências
│   ├── foco-luz-seguidor-001         memorial de ligação revisada
│   └── example/                      protótipos, CAD, zips e referências
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
- [Ligação revisada — S3_CAM_SIM V1.3 + pan-tilt + iluminador LED](docs/foco-luz-seguidor-001)
- [SIMCAM Tracker — Arduino CLI + pan-tilt](docs/example/SIMCAM%20Tracker%20—%20Arduino%20CLI%20%2B%20pan-tilt.md)
- [Ligações e ensaio](docs/example/Ligações%20e%20ensaio.md)
- [AGENTS.md do repositório](../../AGENTS.md)
