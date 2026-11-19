# Componentes de terceiros instalados

Origem: <https://github.com/addyosmani/agent-skills> (MIT, Copyright © 2025 Addy Osmani).
Licença completa: `THIRD_PARTY_LICENSES/addyosmani-agent-skills-LICENSE.txt`.

Foram copiados **9 skills**, **4 checklists de referência** e **3 personas de agente**
para uso local neste subprojeto. Não são código gerado por este repositório; atualizações
devem ser feitas atualizando a fonte e recopiando.

## Skills copiados para `.opencode/skills/`

- `spec-driven-development`
- `planning-and-task-breakdown`
- `incremental-implementation`
- `test-driven-development`
- `code-review-and-quality`
- `security-and-hardening`
- `debugging-and-error-recovery`
- `documentation-and-adrs`
- `constraint-driven-development`

Os skills `security-and-hardening` e `code-review-and-quality` referenciam
`../../references/security-checklist.md` e `../../references/performance-checklist.md`.
Por isso os arquivos foram copiados para `.opencode/references/`, preservando o caminho
relativo. `planning-and-task-breakdown` e `incremental-implementation` usam
`definition-of-done.md`; `test-driven-development` usa `testing-patterns.md`.

## Checklists em `.opencode/references/`

- `definition-of-done.md`
- `testing-patterns.md`
- `performance-checklist.md`
- `security-checklist.md`

## Personas em `.opencode/vendor-agents/`

- `code-reviewer.md`
- `security-auditor.md`
- `test-engineer.md`

As personas são referência para os agentes locais `laserseg-reviewer` e `hardware-auditor`;
elas não são agentes ativos do OpenCode porque `.opencode/vendor-agents/` não é um diretório
de descoberta de agentes. Isso evita expor personas genéricas de web/segurança como opções
de agente neste projeto de firmware.

## Skills Espressif globais (grantwilk/espressif-skills)

Origem: <https://github.com/grantwilk/espressif-skills> (MIT). Licença copiada para
`~/.config/opencode/skills/embedded/espressif/LICENSE` (fora do repositório). Foram
instaladas **39 skills atômicas** para ESP-IDF, esptool, espefuse, OpenOCD, partições,
build, flash, monitoramento e debug. Projeto não oficial, não afiliado à Espressif.
Nenhuma foi copiada para dentro deste repositório.

## Fonte de pinagem LILYGO

Origem: <https://github.com/Xinyuan-LilyGO/T-SimCam> (MIT, repositório oficial).
README.MD, `platformio.ini` e `examples/*/config.h` foram consultados em 2026-11-19
para embasar a skill local `espressif-lilygo`. Nenhum arquivo do upstream foi copiado
para o repositório; apenas fatos e pinagens foram documentados com atribuição.

Observação: o repositório oficial `espressif/skills` (Apache-2.0) foi verificado em
2026-11-19 e estava vazio (somente `.gitkeep`), por isso não foi instalado.

## Promoção para escopo global

A skill de autoria local `espressif-lilygo` também foi instalada globalmente em
`~/.config/opencode/skills/embedded/espressif-lilygo/`, com o mesmo SHA-256 da
cópia deste repositório. Ela continua sendo de autoria do projeto — a cópia
global é apenas conveniência para sessões fora de `firmware/laserseg`.

## Skills locais (não de terceiros)

- `laserseg-context`
- `espressif-lilygo` — costura Espressif + LILYGO com fatos do repo oficial
- `esp32-build-workflow`
- `udp-protocol`
- `hardware-safety`

## Observação

O repositório de origem recomenda `npx skills add addyosmani/agent-skills`. Aqui foi feita
cópia manual porque o ambiente tem a política de execução do PowerShell restritiva para
`npx.ps1`; o resultado é equivalente e mantém os caminhos relativos funcionando.
