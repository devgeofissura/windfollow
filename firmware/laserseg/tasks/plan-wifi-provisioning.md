# Plano de implementação: Provisionamento Wi-Fi (V1)

Spec de origem: `firmware/laserseg/spec/SPEC-wifi-provisioning.md`
(aprovada em 2026-11-19)
Status do plano: proposto — aguardando revisão humana

Este plano é **separado** de `tasks/plan.md` (DM556D + NEMA 23), que segue com
T1, T5, T6 e T7 em aberto. Nenhum arquivo de plano existente foi alterado.

## Visão geral

Criar um módulo compartilhado de provisionamento que permita configurar a rede
pelo navegador: AP `laserseg` no primeiro boot, portal com varredura de SSIDs,
credencial salva em NVS, reboot em STA e fallback para AP (sem retry) quando a
rede salva não é encontrada. O módulo fica pronto para os dois nós, mas só é
ligado à controladora neste ciclo — o sketch de visão de produção ainda não
existe (decisão 5 da spec).

O contrato UDP v0.1, o watchdog de 1200 ms e `LASERSEG_ENABLE_GPIO = 0`
permanecem inalterados.

## Decisões de arquitetura

1. **Dois arquivos de plano.** `plan.md`/`todo.md` continuam com o trabalho
   DM556D; este plano e `todo-wifi-provisioning.md` cuidam do provisionamento.
2. **Lógica pura separada do rádio.** `provisioning_fsm.h` não depende de
   `WiFi.h`: a decisão AP/STA é testável no host e espelhada em Python, como já
   feito para o parser e para `motion_config.py`.
3. **Include relativo, sem biblioteca instalada.** `lib/wifi_provisioning/`
   é incluído por caminho relativo, mesma técnica validada para
   `../controller/parser.cpp`. Nenhuma dependência externa (WiFiManager etc.).
4. **Senha do AP pública por design.** `laserseg` / `123456` fica literal em um
   único `constexpr`, por decisão do usuário. A senha da **rede local salva**
   vai somente para NVS e nunca para arquivo, serial ou log.
5. **Fallback é terminal.** Rede salva ausente ⇒ AP e parada de tentativas.
   Nenhum retry em background.
6. **GPIO continua desligado.** Este ciclo não configura pino de motor, fim de
   curso, luz nem câmera; T5/T6 do plano DM556D seguem bloqueados.

## Lista de tarefas

### Fase 1 — Fundação testável

- [ ] **W1** — Espelho host da máquina de estados
  - Criar `tools/wifi_provisioning_ref.py` e `tests/test_wifi_provisioning.py`.
  - Aceite: NVS vazio → AP; salvo + conectado → STA; salvo + não conectado → AP
    **sem retry**; salvar no AP → reinício → STA; constantes da spec conferidas
    (`laserseg`, `123456`, `15000`, `192.168.4.2`).
  - Verificar: `python.exe -m unittest discover -s tests -p 'test_*.py'` verde.
  - Arquivos: 2. Escopo: S. Depende de: nenhuma.

- [ ] **W2** — Máquina de estados em C++
  - Criar `lib/wifi_provisioning/provisioning_fsm.h` com API sem WiFi e
    `static_assert` das constantes.
  - Aceite: assinatura espelha o referencial Python; um único local define a
    senha do AP; sem dependência de `Arduino.h`.
  - Verificar: compilação do sketch na W4 (o header entra na tradução).
  - Arquivos: 1. Escopo: XS. Depende de: W1.

**Checkpoint Fundação:** suíte host verde; header C++ sem dependência de rádio.

### Fase 2 — Módulo e integração na controladora

- [ ] **W3** — Módulo WiFi/WebServer/DNSServer/Preferences
  - Criar `lib/wifi_provisioning/wifi_provisioning.h` e `.cpp`: AP, portal com
    `WiFi.scanNetworks()`, formulário SSID/senha/IP da controladora, gravação em
    NVS, `ESP.restart()`, fallback AP e logs `PROV ...`.
  - Aceite: só bibliotecas do core; senha da rede nunca em serial/log; fallback
    não agenda nova tentativa; campo de IP com padrão `192.168.4.2`.
  - Verificar: `arduino-cli compile` exit 0; inspeção do diff por segredo.
  - Arquivos: 2. Escopo: M. Depende de: W1, W2.

- [ ] **W4** — Ligar o módulo ao nó DM556D
  - Editar `controller_dm556d/controller_dm556d.ino` para chamar o módulo no
    `setup()`/`loop()`, mantendo autoteste, banner e `LASERSEG_ENABLE_GPIO=0`.
  - Aceite: autoteste `76/76 PASS` na placa; nenhum GPIO tocado; perfil
    `laserseg_dm556d` inalterado.
  - Verificar: detectar placa (`-DetectOnly`), gravar, capturar serial
    (`PROV AP_UP`, `PROV SCAN n=...`, `PROV FALLBACK_AP`).
  - Arquivos: 1. Escopo: S. Depende de: W3.

**Checkpoint Núcleo:** `laserseg-reviewer` aprovou; portal observado em bancada;
nenhuma saída física ativada.

### Fase 3 — Documentação e fechamento

- [ ] **W5** — Atualizar documentação do contrato e estado
  - `docs/PROTOCOLO_UDP.md` (IP em STA), `docs/ESTADO.md`, `README.md` e
    `docs/HARDWARE-DECISIONS.md` (item 6).
  - Aceite: nenhuma afirmação de que credencial de rede vai para o repo;
    UDP 4210 e watchdog 1200 ms continuam documentados como antes.
  - Verificar: leitura cruzada das quatro seções alteradas.
  - Arquivos: 4. Escopo: M. Depende de: W4.

- [ ] **W6** — DEVLOG, revisão e commit seletivo
  - Entrada no `docs/DEVLOG.md`, revisão do `laserseg-reviewer`, `git add`
    explícito e push. **Nunca** `git add -A`: `docs/example01/`,
    `docs/example02/` e o `.zip` pertencem ao usuário.
  - Aceite: commit só com os arquivos deste plano; `git diff --cached` sem a
    senha da rede local (a senha do AP `123456` é esperada e aprovada).
  - Verificar: `git diff --cached --stat` + `git log -1`.
  - Arquivos: 2. Escopo: S. Depende de: W5.

**Checkpoint Final:** todos os critérios de aceite da spec marcados; push feito.

## Riscos e mitigações

| Risco | Impacto | Mitigação |
|---|---|---|
| Scan em modo AP pode não ser suportado em todas as variantes do core | Médio | Testar em bancada na W4; se necessário, usar `WIFI_AP_STA` e confirmar API via context7 antes de codar |
| `tools/save.ps1` pode sinalizar `123456` como segredo | Médio | Commit manual seletivo, como nas entregas anteriores; decisão registrada na spec |
| Dois APs `laserseg` simultâneos confundem o usuário | Médio | Decisão 1 da spec: mesmo SSID, distinguir pela página e pela serial |
| Portal sem autenticação | Médio | Restringir ao provisionamento; rede confiável, como no protótipo |
| Credencial da rede vazar em log | Alto | Nunca imprimir a senha; logging só de SSID/IP nos eventos `PROV ...` |
| Integração com a câmera sem sketch de produção | Baixo | Fora deste ciclo por decisão 5; módulo escrito de forma reutilizável |

## Fora de escopo neste ciclo

- Ligar o módulo ao sketch da câmera (não existe sketch versionado).
- mDNS, broadcast ou DHCP reservado para descobrir a controladora em STA
  (o IP é campo opcional no portal, padrão `192.168.4.2`).
- Qualquer GPIO, motor, fim de curso, LED ou laser.
- Alterações nos arquivos `docs/example01/`, `docs/example02/` e no `.zip`.
