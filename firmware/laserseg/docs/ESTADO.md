# Estado do projeto laserseg

**Atualizado em:** 2026-11-19  
**Fase:** bancada — nó controladora com parser/estado/autoteste/telemetria implementado e compilado; sem GPIO, motores ou luz.

## Situação atual

| Item | Status | Evidência / observação |
|---|---|---|
| Escopo e arquitetura | Definidos | `base-de-conhecimento-rastreamento-com-s3-cam-sim.md`, `foco-luz-seguidor-001` |
| Protótipos de firmware | Existentes em `docs/example01/` e `docs/example02/` | `camera_node.ino`, `controller_drv8834.ino`, `controller_uln2003.ino` |
| Firmware de produção | Parcial | `controller/` existe com parser/estado/autoteste/telemetria (somente serial); `camera_node/` ainda não existe |
| Protocolo UDP | Validado em host e C++ | `PROTOCOLO_UDP.md` + contrato v0.1; 84 testes em `tests/test_protocol.py`, referência em `tools/protocol_ref.py`, vetores C++ gerados por `tools/gen_autotest_vectors.py` |
| Pinout da câmera | Não confirmado | Protótipo usa `CAMERA_MODEL_TTGO_T_CAM_SIM`; sensor pode ser OV2640 ou OV5640 |
| Controladora escolhida | Parcial | ESP32-S3 separada definida; placa exata e GPIOs ainda a validar |
| Motores/driver | Não definidos | Duas alternativas em estudo: bipolar + DRV8834 ou 28BYJ-48 + ULN2003 |
| Foco de luz | Definido como LED difuso | Laser apenas após revisão de segurança |
| Build/flash | Regularizado no host | `arduino-cli` 1.6.0 fora do repo; core `esp32:esp32@3.3.11`; perfil `laserseg` em `tools/flash.ps1` aponta para `controller/controller.ino`; probe de serial segue como fallback |
| Porta serial | Detectada | `COM4`, USB nativo `303A:1001`, ESP32-S3 / 16 MB / MAC `dc:da:0c:28:25:30`; detectar de novo antes de cada gravação |
| MQTT | Operacional | Mosquitto 2.1.4 local + WebSocket + Quick Tunnel; ver `MQTT.md` |
| MCPs | Configurados | `context7` remoto (pode exigir login via `/mcps`); `github` local desabilitado até existir token |
| Skills/agentes | Criados e descobertos | 14 skills locais (5 próprios + 9 instalados), 39 skills Espressif globais, 113 agentes no total, checklists e personas em `.opencode/` |
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
- Skill `espressif-lilygo` criada com pinagem e config do repo oficial
  `Xinyuan-LilyGO/T-SimCam` (MIT) e instalada **local e globalmente** (mesmo SHA-256);
  39 skills `esp-*` instaladas globalmente de `grantwilk/espressif-skills` (MIT).
- `espressif/skills` (oficial, Apache-2.0) foi verificado e está vazio; não instalado.
- Python 3.14.10 LTS instalado (3.15.2 e o stub da Microsoft Store descartados),
  com `PyYAML 6.0.3` e `pyserial 3.5`. Auditoria real de frontmatter com PyYAML:
  **249 skills, 0 erros**. Nova regra do usuário: usar sempre versão LTS/estável.
- Backup dos arquivos editados salvo em `%TEMP%\opencode\backups\` antes da alteração.
- Toolchain validada de ponta a ponta: `firmware/laserseg/serial_probe/serial_probe.ino`
  (somente serial, versionado) compilado com a FQBN `laserseg` e gravado na porta
  detectada; serial confirmou boot e mensagens `laserseg probe: alive`. Nenhum GPIO
  foi acionado. O perfil `tools/flash.ps1 -Sketch laserseg` aponta para esse probe.
- Parser UDP implementado como referência executável e coberto por **84 testes**
  (formato, faixa, sequência, idade em qualquer comando, watchdog com parada
  explícita dos eixos e limites de movimento).
- Revisão independente (`laserseg-reviewer`) encontrou duas falhas de segurança na
  especificação: idade só era checada em `T` e o watchdog não parava os eixos.
  Ambas foram corrigidas antes do commit, com testes de regressão.
- Implementação C++ em `controller/parser.{h,cpp}` espelha a referência. Os
  vetores do autoteste são **gerados** a partir de `protocol_ref.py`, não copiados
  à mão. Compilação com a FQBN `laserseg`: `exit=0`, 314816 bytes de flash (10%)
  e 22856 bytes de RAM (6%). Telemetria pura `ACK,<seq>,<state>,<fault>,<light>`
  documentada em `PROTOCOLO_UDP.md`.
- Firmware gravado em `COM4` (detectada na hora) e validado **na placa**:
  `AUTOTEST total=76 passed=76 failed=0` / `AUTOTEST PASS`, com heartbeats
  `laserseg controller parser: alive`. O teste em dispositivo encontrou um bug
  que o host não via: a telemetria emitia `none`/`range`/`link` em minúsculas,
  contrariando o contrato `NONE`/`RANGE`/`LINK`. Corrigido com mapa exclusivo
  para telemetria e o autoteste agora mostra `expected`/`got` em caso de falha.
- Auditoria de hardware aprovou a gravação deste bloco **somente serial** e listou
  seis condições obrigatórias antes de acionar GPIO (`PROTOCOLO_UDP.md`,
  "Condições obrigatórias antes de acionar hardware"): tratar `fault()` como
  parada+luz OFF, chamar `poll()` antes dos motores, nunca usar `now_ms=-1` em
  campo, corrigir wrap-around do `millis()`, fechar o checklist elétrico e
  restringir o UDP sem autenticação à bancada.

## Decisões registradas

1. Manter a T-SIMCAM como nó de visão; não acionar motores/iluminador diretamente por ela.
2. Enviar apenas posição/erro e comandos por UDP; não transmitir vídeo para outro computador.
3. Usar `AccelStepper` na controladora, com fins de curso NF e timeout de link.
4. Tratar `docs/example01/` como fonte de ideias validadas apenas por compilação, não por ensaio elétrico.
5. Preferir LED difuso a laser no primeiro protótipo.
6. Qualquer mudança de firmware deve atualizar este arquivo, o protocolo e o `DEVLOG.md`.

## Pendências priorizadas

1. Re-detectar a placa (cabo/porta/driver) antes de qualquer gravação.
2. Confirmar o sensor instalado na S3_CAM_SIM V1.3 e o pinout do esquema da revisão.
3. Escolher a controladora ESP32-S3, placa de driver e motores com base em torque/corrente reais.
4. Validar em bancada a FQBN `app3M_fat9M_16MB` já confirmada em `board details`; não misturar com o perfil `windfollow` da raiz (`huge_app`).
5. Instrumentar o link UDP com sequência/idade e política de reconexão antes de uso fora de bancada.
6. Definir homing/limites mecânicos reais; os ±60° atuais são apenas estimativa a partir do centro manual.
7. Para o MQTT público, migrar do Quick Tunnel para Named Tunnel com dominio proprio antes de sair da bancada.

## Próximos passos sugeridos

1. Fechar as seis condições de integração listadas em `PROTOCOLO_UDP.md` antes de
   adicionar GPIO, motores ou LED.
2. Transformar os protótipos em sketches de produção, um nó por pasta.
3. Fazer ensaio de bancada sem carga mecânica, seguindo `docs/example01/Ligações e ensaio.md`.
4. Só então integrar câmera + controladora + foco com limites conservadores.

## Referência de sessão atual

- `tools/flash.ps1 -DetectOnly` encontrou `COM4` (`VID 303A:1001`, USB nativo),
  ESP32-S3 com 16 MB e MAC `dc:da:0c:28:25:30` — compatível com o perfil `laserseg`.
- Não existe porta fixa: o valor acima é apenas o resultado desta detecção.
- O sensor da câmera e o driver de motor ainda não foram confirmados fisicamente.
