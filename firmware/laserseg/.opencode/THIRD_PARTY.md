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

## Skills locais (não de terceiros)

- `laserseg-context`
- `esp32-build-workflow`
- `udp-protocol`
- `hardware-safety`

## Observação

O repositório de origem recomenda `npx skills add addyosmani/agent-skills`. Aqui foi feita
cópia manual porque o ambiente tem a política de execução do PowerShell restritiva para
`npx.ps1`; o resultado é equivalente e mantém os caminhos relativos funcionando.
