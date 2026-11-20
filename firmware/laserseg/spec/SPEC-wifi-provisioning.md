# Spec Delta: Provisionamento Wi-Fi (AP → STA com portal de varredura) — V1

Status: aprovada em 2026-11-19

Especificação delta de `SPEC-dm556d-nema23.md` (aprovada em 2026-11-19).
Origem: resposta do usuário ao item 6 de `docs/HARDWARE-DECISIONS.md`,
complementada em 2026-11-19. Nada desta spec altera o contrato UDP v0.1.

## Objective

Permitir que **ambos os nós** (câmera T-SIMCAM e controladora ESP32-S3) sejam
provisionados sem acesso serial: ao ligar, o dispositivo sobe um Access Point,
o usuário conecta, abre a página inicial, vê a lista de redes varridas, escolhe
uma e informa a senha. O dispositivo salva a credencial, reinicia e passa a
operar como cliente na rede local. Se a rede salva não for encontrada, ele
volta ao AP e **para** — sem tentativas em background.

Resultado esperado: configuração de rede feita pelo navegador, sem recompilar e
sem digitar credencial na serial.

## Current Behavior

- `controller/` e `controller_dm556d/` são **serial-only**: não incluem
  `WiFi.h`, não têm WebServer e não gravam credencial alguma.
- Os únicos exemplos de provisionamento vivem em `docs/example01/` e
  `docs/example02/` (protótipos não validados): AP fixo `SIMCAM-SETUP` com
  senha `simcam123` e `NET_MODE` compilado em 0/1 — não é reutilizável.
- `PROTOCOLO_UDP.md` fixa AP `192.168.4.1` (câmera) / `192.168.4.2`
  (controladora) e UDP `4210`, sem autenticação.
- `LASERSEG_ENABLE_GPIO = 0`: nenhum pino de motor, fim de curso ou luz é
  configurado em firmware validado.

## New Behavior

1. **Módulo compartilhado** `lib/wifi_provisioning/` (API + máquina de estados
   pura + implementação WiFi/WebServer/DNSServer/Preferences), incluído por
   caminho relativo nos dois nós — mesma técnica já validada para
   `../controller/parser.cpp`, sem copiar a lógica.
2. **Senha do AP pública por design; credencial da rede salva fora do repo**:
   na primeira inicialização (NVS vazio) o dispositivo publica o AP `laserseg`
   com a senha `123456` gravada no código versionado — decisão final do usuário
   em 2026-11-19, porque essa senha precisa ser conhecida por quem for
   configurar o aparelho. A senha da rede local escolhida no portal fica
   somente em NVS. O AP escuta em `192.168.4.1`.
3. **Portal com varredura**: ao abrir `http://192.168.4.1`, a página lista as
   redes encontradas por `WiFi.scanNetworks()`; o usuário escolhe um SSID e
   digita a senha. O SSID e a senha escolhidos são gravados **somente em NVS**
   (`Preferences`), nunca em arquivo, log ou serial.
4. **Troca para rede local**: após salvar, o dispositivo reinicia, tenta a rede
   gravada e, se conectar, passa a operar em STA. O IP é obtido por DHCP.
5. **Fallback "AP e para"**: se a rede gravada não for encontrada ou não
   conectar dentro do timeout, o dispositivo sobe o AP `laserseg` novamente e
   **não** continua tentando em background. Só uma nova configuração pelo
   portal (ou reinício com credencial válida) muda esse estado.
6. **Nada muda no contrato v0.1**: porta `4210`, mensagens `TRACK/LIGHT/MANUAL/T`,
  sequência/idade, `MAX_TRAVEL`, `DEAD_BAND` e watchdog de `1200 ms` permanecem idênticos.
7. **Nada muda em GPIO**: `LASERSEG_ENABLE_GPIO` continua `0`; T5/T6 seguem
   bloqueados até a auditoria de hardware.

## Tech Stack

- Arduino-ESP32 3.3.11 (core já instalado), C++17.
- Apenas bibliotecas do core: `WiFi.h`, `WebServer.h`, `DNSServer.h`,
  `Preferences.h`. **Nenhuma dependência externa nova** (WiFiManager etc.).
- Host: Python 3.14 LTS em `%LOCALAPPDATA%\Programs\Python\Python314\python.exe`
  para testes de referência da máquina de estados.

## Commands

```powershell
# Testes host (parser + movimento + provisionamento)
powershell -ExecutionPolicy Bypass -File firmware/laserseg/tests/run.ps1

# Compilação (build fora do repo, FQBN do perfil laserseg_dm556d)
& "$env:LOCALAPPDATA\Programs\arduino-cli\arduino-cli.exe" compile `
  --fqbn 'esp32:esp32:esp32s3:UploadSpeed=921600,USBMode=hwcdc,CDCOnBoot=cdc,CPUFreq=240,FlashSize=16M,PartitionScheme=app3M_fat9M_16MB,PSRAM=opi' `
  --build-path (Join-Path $env:TEMP 'laserseg-dm556d-build') `
  'E:\dev\windfollow\firmware\laserseg\controller_dm556d'

# Detecção obrigatória antes de qualquer gravação
powershell -ExecutionPolicy Bypass -File tools/flash.ps1 -Sketch laserseg_dm556d -DetectOnly
```

## Project Structure

```text
firmware/laserseg/
  lib/wifi_provisioning/            # NOVO — módulo compartilhado
    wifi_provisioning.h             # API, constantes e senha do AP
    wifi_provisioning.cpp           # WiFi + WebServer + DNS + NVS
    provisioning_fsm.h              # decisão pura, sem dependência de WiFi
  controller_dm556d/                # inclui o módulo (ainda sem GPIO)
  controller/                       # baseline serial-only, permanece intacto
  tests/test_wifi_provisioning.py   # NOVO — referência host da FSM
  tools/wifi_provisioning_ref.py    # NOVO — espelho puro da decisão
  docs/PROTOCOLO_UDP.md             # seção de rede atualizada (IP em STA)
  docs/ESTADO.md                    # estado do módulo
  spec/SPEC-wifi-provisioning.md    # esta spec
```

A câmera de produção ainda não existe como sketch versionado — hoje só há
protótipos em `docs/example01/` e `docs/example02/`. O módulo será escrito para
os dois nós, mas a ligação no nó de visão depende da criação desse sketch.

## Code Style

```cpp
// lib/wifi_provisioning/wifi_provisioning.h
namespace laserseg {

// Senha do AP de provisionamento — exceção explícita aprovada em 2026-11-19.
static constexpr char kApSsid[] = "laserseg";
static constexpr char kApPassword[] = "123456";
static constexpr uint32_t kApTimeoutMs = 15000;

// Sem WiFi na assinatura: testável no host e no autoteste.
enum class ProvState { kAp, kSta, kSaved };
ProvState nextState(ProvState current, bool saved, bool connected);

}  // namespace laserseg
```

Constantes da spec replicadas em `tools/wifi_provisioning_ref.py` e conferidas
por teste, como já feito para `motion_config.py` / `driver_config.h`.

## Testing Strategy

- **Host (`unittest`)**: tabela de decisão da FSM — NVS vazio → `kAp`; salvo e
  conectado → `kSta`; salvo e não conectado → `kAp` **sem retry**; transição
  de `kAp` após salvar → `kSta`. Constantes espelhadas conferidas por regex.
- **Compile time**: `static_assert` nos limites e na senha do AP (um único local).
- **Em bancada (sem GPIO)**: placa detectada, sketch gravado, serial mostra
  `PROV AP_UP`, `PROV SCAN n=<N>`, `PROV SAVED`, `PROV STA_CONNECTED ip=<ip>` ou
  `PROV FALLBACK_AP`. Senha da rede **nunca** aparece na serial.
- **Manual**: conectar no AP `laserseg` do celular, abrir `192.168.4.1`,
  conferir lista de SSIDs, escolher a rede local e confirmar o salto para STA.

## Boundaries

Deltas em relação à spec original; o restante é herdado.

- **Always:** backup antes de substituir arquivo; detecção de placa antes de
  gravar; testes verdes antes de commit; commit seletivo — **nunca** `git add -A`
  enquanto existirem as alterações do usuário em `docs/example01/`,
  `docs/example02/` e no `.zip`.
- **Ask first:** mudar porta/formato UDP, adicionar biblioteca externa, tocar
  GPIO/motor/luz, alterar o SSID ou a senha do AP depois desta spec, e integrar
  o módulo no sketch da câmera quando ele existir.
- **Never:** gravar a senha da **rede local salva** em arquivo, serial ou log;
  habilitar retry em background no fallback; ativar `LASERSEG_ENABLE_GPIO`;
  alterar o watchdog de `1200 ms`. A senha do AP (`123456`) é pública por
  design e fica no código — não se aplica a ela a regra de segredo.

### Exceção de segurança registrada

A senha do AP de provisionamento será **literal no código**, por decisão
explícita do usuário em 2026-11-19 — exceção pontual à regra "nenhum segredo no
repo". Consequências aceitas: é uma senha fraca e publicamente conhecida;
qualquer pessoa ao alcance do AP pode abrir o portal durante o provisionamento.
Mitigações: fica em **um único** `constexpr`, é usada **somente** no AP de
provisionamento, e a credencial da rede real continua apenas em NVS. O usuário
decidiu explicitamente que **`123456` é definitiva nesta V1** e deve constar no
código e no git, sem recomendação de troca: é uma credencial que quem configura
o aparelho precisa saber. O `tools/save.ps1` pode sinalizar o diff; o commit
será revisado manualmente como já feito nas entregas anteriores.

## Success Criteria

- [ ] Com NVS vazio, AP `laserseg` aparece e `http://192.168.4.1` lista redes varridas.
- [ ] Ao salvar pelo portal, a credencial vai para NVS e o dispositivo reinicia em STA.
- [ ] O AP usa exatamente `laserseg` / `123456`, sem geração aleatória.
- [ ] Rede salva ausente ⇒ AP `laserseg` novamente e nenhuma tentativa em background.
- [ ] Serial mostra `PROV FALLBACK_AP` e nunca a senha informada no formulário.
- [ ] Contrato UDP v0.1 inalterado: testes do parser seguem 84/84 e autoteste 76/76.
- [ ] Suíte host permanece verde (92 testes atuais + novos testes de FSM).
- [ ] `LASERSEG_ENABLE_GPIO = 0`; nenhum GPIO, motor ou luz ativado.
- [ ] `git diff --cached` não contém a senha da rede salva.

## Out of Scope (Deferred)

- Descoberta do IP da controladora em STA (mDNS/broadcast/DHCP reservado):
  o protocolo hoje assume `192.168.4.2` apenas no AP. Ver pergunta aberta 2.
- Senha aleatória por dispositivo ou rotação de credencial de provisionamento.
- Autenticação no portal web e no UDP (rede confiável, como no protótipo).
- WPA3 Enterprise, banda 5 GHz e provisionação por Bluetooth/BLE.
- Chave de emergência e qualquer saída física — gate T5.

## Decisões fechadas na aprovação (2026-11-19)

1. **SSID duplicado:** manter `laserseg` nos dois nós; distinguir pela página
   e pela serial. Não usar sufixos nesta V1.
2. **IP da controladora em STA:** campo opcional "IP da controladora" no
   portal, com padrão `192.168.4.2`.
3. **Tempo limite STA:** `15000 ms` (`kApTimeoutMs`).
4. **Após salvar:** `ESP.restart()` automático.
5. **Câmera de produção:** o módulo fica pronto para os dois nós, mas a
   integração no sketch de visão só ocorre quando esse sketch existir.

Nenhuma pergunta em aberto resta nesta V1.
