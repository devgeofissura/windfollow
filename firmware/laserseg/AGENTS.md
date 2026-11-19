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
| Trabalhar com ESP32/S3, Espressif ou LILYGO T-SimCam | `espressif-lilygo` |
| Ferramenta Espressif específica (idf.py, esptool, fuses, debug) | `esp-toolchain-overview` → skill `esp-*` correspondente |
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

## Especialistas disponíveis

- **Skill Espressif + LILYGO (local e global):** `espressif-lilygo` — carregue
  antes de qualquer tarefa ESP32/S3 ou T-SimCam. Cópia local em
  `.opencode/skills/espressif-lilygo/`; cópia global em
  `~/.config/opencode/skills/embedded/espressif-lilygo/` (mesmo SHA-256). Se
  atualizar uma, atualize a outra.
- **Skills globais Espressif (39):** iniciar com `esp-toolchain-overview`; depois
  `esp-detect-device`, `esp-chip-info`, `esp-idf-build`, `esp-flash-project`,
  `esp-troubleshoot`, `esp-decode-crash`. Instaladas em
  `~/.config/opencode/skills/embedded/espressif/` (MIT).
- **Skills globais do kit:** `esp32-development`, `arduino-development`,
  `rtos-basics`, `mqtt-protocol`, `sensor-integration`, `robotics`.
- **Agentes globais:** `embedded-engineer` (firmware C/C++, RTOS, drivers),
  `vision-engineer` (visão/OpenCV), `robotics-engineer` (motores/controle),
  `security-engineer` e `code-reviewer` (revisão).
- **Agentes locais:** `laserseg-dev`, `laserseg-reviewer`, `hardware-auditor`.

Para tarefa de firmware, carregue `espressif-lilygo` antes das skills atômicas e
adote `embedded-engineer` como persona; para visão, `vision-engineer`; antes de
qualquer gravação, `hardware-auditor`.

## Ferramentas de apoio (regra do usuário)

- **Python: sempre LTS/estável, nunca a versão mais recente.** Instalada
  **Python 3.14.10 (64-bit)** em `%LOCALAPPDATA%\Programs\Python\Python314\`
  (`pip 26.2.2`). O stub da Microsoft Store e a versão 3.15.2 foram descartados.
  O PATH do usuário foi ajustado para o interpretador LTS (backup do PATH original
  em `%TEMP%\opencode\backups\`); ainda assim use o caminho absoluto em scripts críticos.
- Pacotes: `PyYAML 6.0.3` (auditoria de frontmatter) e `pyserial 3.5` (portas).
- Auditoria de frontmatter com parser real em `%TEMP%\opencode\yaml-check\validate.py`:
  **249 skills auditadas, 0 erros** (locais + Espressif + kit). Recriar o script
  se a pasta temporária for limpa; nunca aceitar "procurei por `---`" como prova.

## Backup e banco de dados (regra global do usuário)

- **Nunca sobrescreva nem apague código sem backup.** Antes de substituir ou remover
  qualquer arquivo/trecho: leia o conteúdo atual, preserve cópia (fora do repositório
  ou em commit identificado), revise o diff e só então aplique a mudança.
- **Cuidado com migrations.** Antes de qualquer mudança de schema: inspecione os dados
  atuais, faça backup verificável e tenha rollback pronto. `DROP`, `TRUNCATE`,
  `DELETE` e alterações destrutivas exigem confirmação explícita do usuário e backup
  testado. Em dúvida, pare e pergunte.

## Antes de gravar

1. Rodar detecção da raiz: `powershell -ExecutionPolicy Bypass -File tools/flash.ps1 -DetectOnly`.
2. Confirmar chip, flash e MAC; nunca usar COM fixo.
3. `laserseg` ainda não tem perfil em `tools/flash.ps1` — adicionar perfil ou usar
   `arduino-cli` com FQBN explícito e build path em `$env:TEMP` (ver `esp32-build-workflow`).
4. Só então compilar e gravar, com o foco desligado e os eixos centralizados manualmente.

## Ao concluir

Seguir `tools/save.ps1` da raiz: uma tarefa = um commit com entrada no `DEVLOG.md` e push.
