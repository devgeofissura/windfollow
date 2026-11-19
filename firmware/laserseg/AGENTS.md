# AGENTS.md — regras específicas do laserseg

Este arquivo complementa o [`AGENTS.md`](../../AGENTS.md) da raiz. Em caso de conflito,
valem as regras da raiz (detecção de placa, `tools/save.ps1`, porta COM nunca fixa),
exceto onde este arquivo for mais restritivo.

## Intuição primeiro: intenção → skill

Antes de agir, carregue a skill que corresponde à intenção. Se casar com mais de uma,
carregue da mais geral para a mais específica.

| Intenção | Skill |
|---|---|
| Começar qualquer tarefa no laserseg, esclarecer escopo ou estado | `laserseg-context` |
| Requisito novo, mudança de arquitetura, spec antes de código | `spec-driven-development` |
| Dividir trabalho em passos pequenos | `planning-and-task-breakdown` |
| Implementar firmware, um nó por vez | `incremental-implementation` |
| Escrever teste de parser/comportamento | `test-driven-development` |
| Compilar, detectar placa, FQBN, gravar | `esp32-build-workflow` |
| Mexer em UDP, mensagens `TRACK/LIGHT/MANUAL/T` | `udp-protocol` |
| Mexer em GPIO, motor, fin de curso, LED/laser, alimentação | `hardware-safety` |
| Revisar código ou documentação | `code-review-and-quality` |
| Ameaça de segurança, input não confiável, segredos, hardening | `security-and-hardening` |
| Bug inesperado, falha de compilação, comportamento estranho | `debugging-and-error-recovery` |
| Atualizar README, protocolo, ADR ou estado | `documentation-and-adrs` |
| Definir limites/critérios de aceite que não podem ser violados | `constraint-driven-development` |

Regras de uso:

- Se uma skill foi carregada, siga o fluxo dela até o critério de saída; não aplique só o título.
- Em mudanças de firmware, a ordem mínima é: `laserseg-context` → `spec-driven-development`
  (quando não há spec) → `test-driven-development` → `incremental-implementation` →
  `esp32-build-workflow` → `hardware-safety` se houver GPIO envolvido.
- Para respostas rápidas de estado, carregue apenas `laserseg-context` e responda em
  3–6 linhas. Não carregue o repositório inteiro "por precaução".

## Agentes

- `laserseg-dev` (padrão): implementação e documentação. Não grava hardware sem detecção prévia.
- `laserseg-reviewer` (subagente, somente leitura): revisão de código, protocolo e docs.
- `hardware-auditor` (subagente, somente leitura): auditoria elétrica, GPIO e segurança de luz.

Use `laserseg-reviewer` antes de commitar qualquer mudança de firmware e `hardware-auditor`
antes de qualquer mudança que toque pinagem, alimentação ou saída de luz.

## MCPs

- `context7` — documentação de bibliotecas (ESP32, Arduino, AccelStepper). Use quando houver
  dúvida de API; não confie em memória para assinatura de função.
- `github` — configurado, mas `disabled: true`; habilitar só com
  `GITHUB_PERSONAL_ACCESS_TOKEN` no ambiente. Nunca colocar token no repositório.

## Fontes e ressalvas

`docs/example/` é protótipo de referência, não firmware validado. Sensor da S3_CAM_SIM
(OV2640 vs OV5640) e revisão do pinout ainda não foram confirmados. Se uma decisão depender
disso, pare e pergunte em vez de assumir.

Nenhum LED/laser deve ligar no boot, no reset, na perda de câmera ou no watchdog. Motores
devem parar com fin de curso NF, comando inválido ou link expirado (> 1200 ms).

## Antes de gravar

1. Rodar detecção da raiz: `powershell -ExecutionPolicy Bypass -File tools/flash.ps1 -DetectOnly`.
2. Confirmar chip, flash e MAC; nunca usar COM fixo.
3. `laserseg` ainda não tem perfil em `tools/flash.ps1` — adicionar perfil ou usar
   `arduino-cli` com FQBN explícito e build path em `$env:TEMP` (ver `esp32-build-workflow`).
4. Só então compilar e gravar, com o foco desligado e os eixos centralizados manualmente.

## Ao concluir

Seguir `tools/save.ps1` da raiz: uma tarefa = um commit com entrada no `DEVLOG.md` e push.
