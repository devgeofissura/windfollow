# Estado do projeto laserseg

**Atualizado em:** 2026-11-19  
**Fase:** fundação (documentação + automação OpenCode). Nenhum firmware de produção ainda.

## Situação atual

| Item | Status | Evidência / observação |
|---|---|---|
| Escopo e arquitetura | Definidos | `base-de-conhecimento-rastreamento-com-s3-cam-sim.md`, `foco-luz-seguidor-001` |
| Protótipos de firmware | Existentes em `docs/example/` | `camera_node.ino`, `controller_drv8834.ino`, `controller_uln2003.ino` |
| Firmware de produção | Não criado | Pastas `camera_node/`, `controller_*` ainda não existem |
| Protocolo UDP | Documentado | `PROTOCOLO_UDP.md`; implementação apenas nos protótipos |
| Pinout da câmera | Não confirmado | Protótipo usa `CAMERA_MODEL_TTGO_T_CAM_SIM`; sensor pode ser OV2640 ou OV5640 |
| Controladora escolhida | Parcial | ESP32-S3 separada definida; placa exata e GPIOs ainda a validar |
| Motores/driver | Não definidos | Duas alternativas em estudo: bipolar + DRV8834 ou 28BYJ-48 + ULN2003 |
| Foco de luz | Definido como LED difuso | Laser apenas após revisão de segurança |
| Build/flash | Não regularizado | `arduino-cli` ausente do PATH; `laserseg` sem perfil em `tools/flash.ps1` |
| Porta serial | Provisória | `COM3`, USB-SERIAL CH340; chip/flash/MAC não verificados |
| MCPs | Configurados | `context7` remoto (pode exigir login via `/mcps`); `github` local desabilitado até existir token |
| Skills/agentes | Criados e descobertos | 13 skills (4 próprios + 9 instalados), 3 agentes, checklists e personas em `.opencode/` |
| Config OpenCode | Validada | `opencode debug config` e `opencode debug agents` reconhecem arquivos do subprojeto |
| Memória de sessão | Criada | `docs/history.md` resume aprendizados e commits em ~166 linhas |

## Validação da fundação (2026-11-19)

- `opencode debug config` aponta `.opencode/opencode.jsonc` com `default_agent`,
  `instructions` e os dois MCPs.
- `opencode debug agents` lista `laserseg-dev`, `laserseg-reviewer` e `hardware-auditor`
  com os modos primary/subagent corretos.
- `opencode mcp list` reportou "No MCP servers configured" mesmo com a config carregada;
  tratar como limitação do CLI v2.3.2 para servers de projeto. Usar `/mcps` na TUI para
  conectar/autenticar o `context7`.
- Nenhum segredo foi escrito nos arquivos de configuração; o token do GitHub é lido de
  `GITHUB_PERSONAL_ACCESS_TOKEN` somente quando o servidor for habilitado.

## Decisões registradas

1. Manter a T-SIMCAM como nó de visão; não acionar motores/iluminador diretamente por ela.
2. Enviar apenas posição/erro e comandos por UDP; não transmitir vídeo para outro computador.
3. Usar `AccelStepper` na controladora, com fins de curso NF e timeout de link.
4. Tratar `docs/example/` como fonte de ideias validadas apenas por compilação, não por ensaio elétrico.
5. Preferir LED difuso a laser no primeiro protótipo.
6. Qualquer mudança de firmware deve atualizar este arquivo, o protocolo e o `DEVLOG.md`.

## Pendências priorizadas

1. Instalar/localizar `arduino-cli` e confirmar a versão do core ESP32 (exemplos citam `esp32:esp32@3.3.12`).
2. Confirmar o sensor instalado na S3_CAM_SIM V1.3 e o pinout do esquema da revisão.
3. Escolher a controladora ESP32-S3, placa de driver e motores com base em torque/corrente reais.
4. Fechar FQBN: os exemplos usam `PartitionScheme=app3M_fat9M_16MB`, enquanto o perfil `windfollow` da raiz usa `huge_app`. Não misturar sem validar.
5. Adicionar perfil `laserseg` ao `tools/flash.ps1` antes de qualquer gravação.
6. Instrumentar o link UDP com sequência/idade e política de reconexão antes de uso fora de bancada.
7. Definir homing/limites mecânicos reais; os ±60° atuais são apenas estimativa a partir do centro manual.

## Próximos passos sugeridos

1. Transformar os protótipos em sketches de produção, um nó por pasta.
2. Escrever testes de parse do protocolo com mensagens sintéticas, antes de ligar motores.
3. Validar compilação de cada nó com `arduino-cli` em diretório temporário.
4. Fazer ensaio de bancada sem carga mecânica, seguindo `docs/example/Ligações e ensaio.md`.
5. Só então integrar câmera + controladora + foco com limites conservadores.

## Referência de sessão atual

- Detecção automática via `tools/flash.ps1 -DetectOnly` falhou: `arduino-cli` não está no PATH.
- Porta serial listada pelo Windows: `COM3` (`USB\VID_1A86&PID_7523`, CH340).
- Compatibilidade com `laserseg` não pode ser afirmada: não há perfil nem identificação de chip.
