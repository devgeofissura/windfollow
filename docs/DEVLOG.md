# Diário de desenvolvimento (DEVLOG)

Registro cronológico do que foi feito em cada commit, com as dificuldades
encontradas e como foram resolvidas. Cada entrada é inserida automaticamente no
topo da seção **Histórico** por `tools/save.ps1`, que commita e faz push.

Formato de cada entrada: data/hora, mensagem do commit, o que mudou, dificuldades
e soluções, arquivos tocados.

---

## Histórico

### 2026-11-15 19:35 - feat(irobot): sensores AHT20/BMP280/VL53L0X e modo autonomo

**Area:** irobot

**Feito:**
- SensorService le AHT20 + BMP280 (temp/umidade/pressao) e VL53L0X (distancia) no I2C SDA=21/SCL=22 com intervalos escalonados
- AutoNavigator implementa FSM frente/para/desvia/assenta/desiste com giros alternados a cada 650 ms e limite de 3 tentativas
- novas rotas GET /api/sensors, GET e POST /api/auto e comandos seriais a e m
- painel ganhou card Ambiente com distancia e botao Autonomo e a pagina /teste mostra linha compacta com toggle
- WebAssets.h regenerado e firmware compila em 81% de flash

**Dificuldades e solucoes:**
- arduino-cli reportou sucesso com 1.070.207 bytes
- a gravacao nao pode ser feita porque nenhuma porta COM estava presente no momento do teste -> deixei o firmware compilado e orientei reconectar o ESP32 e gravar com tools/flash.ps1 -Sketch irobot

**Arquivos:**
- `firmware/irobot/DashboardService.cpp`
- `firmware/irobot/DashboardService.h`
- `firmware/irobot/HardwareConfig.h`
- `firmware/irobot/WebAssets.h`
- `firmware/irobot/irobot.ino`
- `firmware/irobot/web/css/style.css`
- `firmware/irobot/web/index.html`
- `firmware/irobot/web/js/app.js`
- `firmware/irobot/web/teste.html`
- `firmware/irobot/AutoNavigator.cpp`
- `firmware/irobot/AutoNavigator.h`
- `firmware/irobot/SensorService.cpp`
- `firmware/irobot/SensorService.h`

### 2026-11-15 19:08 - docs(irobot): registra o Prototipo-67 e o teste com duas baterias 3.7V

**Area:** irobot

**Feito:**
- README ganha secao Prototipos com o Prototipo-67
- teste com 2 baterias 3.7V em serie (7.4V) alimentou tudo, ficou um pouco fraco mas validou todas as funcionalidades

**Dificuldades e solucoes:**
- nenhuma

**Arquivos:**
- `firmware/irobot/README.md`

### 2026-11-15 18:53 - docs(irobot): registra alimentacao do ESP32 pela saida 5V do L298N

**Area:** irobot

**Feito:**
- README documenta bateria 7-12V no L298N com jumper 5V-EN, saida 5V para VIN do ESP32 e GND comum
- aviso de nao usar USB junto
- menciona a rota /teste

**Dificuldades e solucoes:**
- nenhuma

**Arquivos:**
- `firmware/irobot/README.md`

### 2026-11-15 17:12 - feat(irobot): painel antigo de volta em / e versao simples em /teste

**Area:** irobot

**Feito:**
- raiz volta a ter joystick a esquerda e sliders de velocidade a direita
- /teste serve a pagina so com 5 botoes
- gerador embute teste_html e links antigos ganham ?v=3 contra cache

**Dificuldades e solucoes:**
- esp32 parou de responder ao esptool apos matar o monitor serial -> reset fisico na placa e nova deteccao da porta resolveram

**Arquivos:**
- `firmware/irobot/DashboardService.cpp`
- `firmware/irobot/DashboardService.h`
- `firmware/irobot/WebAssets.h`
- `firmware/irobot/tools/generate_web_assets.ps1`
- `firmware/irobot/web/css/style.css`
- `firmware/irobot/web/index.html`
- `firmware/irobot/web/js/app.js`
- `firmware/irobot/web/teste.html`

### 2026-11-15 16:50 - fix(irobot): gerador de WebAssets embutia aspas no inicio do app.js

**Area:** irobot

**Feito:**
- generate_web_assets.ps1 nao adiciona mais aspas extras apos R-raw
- JS servido voltou a ser valido e os botoes conseguem chamar a API
- WebAssets.h regerado e gravado em COM5

**Dificuldades e solucoes:**
- app.js chegava ao navegador com aspas solta no inicio (SyntaxError) e nenhum fetch saia do celular -> corrigido o delimitador do raw string e conferido byte a byte no header gerado

**Arquivos:**
- `firmware/irobot/DashboardService.cpp`
- `firmware/irobot/WebAssets.h`
- `firmware/irobot/tools/generate_web_assets.ps1`

### 2026-11-15 16:17 - feat(irobot): tela simplificada so com botoes de direcao

**Area:** irobot

**Feito:**
- index.html com 5 botoes grandes (frente, tras, esquerda, direita, parar)
- app.js reescrito mostrando erro de API na propria tela
- css novo e WebAssets.h regerado

**Dificuldades e solucoes:**
- sem acesso ao AP da placa pela maquina (Windows bloqueia netsh wlan connect sem Localizacao) -> validacao feita por compilacao, gravacao e log serial de boot

**Arquivos:**
- `firmware/irobot/WebAssets.h`
- `firmware/irobot/web/css/style.css`
- `firmware/irobot/web/index.html`
- `firmware/irobot/web/js/app.js`

### 2026-11-15 16:06 - fix(irobot): tela nao enviava comandos por ler o corpo do POST do socket

**Area:** irobot

**Feito:**
- POST usa server.arg('plain') em move e config
- o corpo ja vem bufferizado pelo WebServer e readString devolvia vazio esperando 1s por requisicao

**Dificuldades e solucoes:**
- handlers liam client().readString() depois do parser consumir o corpo, causando 400 e travando o servidor -> trocado por arg('plain') e regravado no ESP32

**Arquivos:**
- `firmware/irobot/DashboardService.cpp`

### 2026-11-15 15:58 - fix(tooling): flash.ps1 falhava com uma unica porta serial

**Area:** tooling

**Feito:**
- ports agora e sempre array com @() e a gravacao segue sem -Port
- firmware irobot compilado e gravado com sucesso em COM8 (ESP32-D0WD-V3)

**Dificuldades e solucoes:**
- PowerShell desembrulha array de um elemento e .Count fica vazio -> Get-SerialPorts embrulhado em @() e campo Count validado

**Arquivos:**
- `tools/flash.ps1`

### 2026-11-15 15:53 - docs(devlog): corrige entrada quebrada pelo separador de itens

**Area:** docs, raiz

**Feito:**
- entrada do commit anterior reorganizada em bullets
- AGENTS.md avisa para nao usar ponto e virgula dentro do texto de um item

**Dificuldades e solucoes:**
- o proprio texto do item continha o separador e foi dividido ao meio -> texto reescrito sem o caractere

**Arquivos:**
- `AGENTS.md`
- `docs/DEVLOG.md`

### 2026-11-15 15:52 - fix(tooling): itens de Summary/Difficulty separados por ';' no save.ps1

**Area:** docs, raiz, tooling

**Feito:**
- save.ps1 divide -Summary e -Difficulty pelo ponto e virgula (powershell -File nao converte virgula em lista)
- entrada anterior do DEVLOG reorganizada em bullets
- AGENTS.md atualizado com o separador

**Dificuldades e solucoes:**
- powershell.exe -File entregou "a","b" como uma unica string e o DEVLOG saiu corrido -> split pelo separador e entrada corrigida

**Arquivos:**
- `AGENTS.md`
- `docs/DEVLOG.md`
- `tools/save.ps1`

### 2026-11-15 15:51 - feat(irobot): chassi 2 rodas + caster, deteccao automatica de placa/COM e DEVLOG automatico

**Area:** irobot, windfollowblind, tooling, docs

**Feito:**
- chassi documentado como 2 motores (1 roda cada) + roda caster livre
- novo `tools/flash.ps1`: detecta COM na hora, confere chip com esptool e grava
- novo `tools/save.ps1` + `docs/DEVLOG.md`: commit, push e diário automáticos
- `AGENTS.md`: fluxo de início de sessão e regras de porta/commit
- corrige `windfollowblind.ino` (não compilava) e typos de mDNS do blind
- remove COM fixa dos READMEs/documentação; README raiz ganha seção irobot

**Dificuldades e soluções:**
- arduino-cli marca clone CH340 como Unknown -> chip identificado via esptool flash-id
- `windflollowblind.ino` não batia com a pasta (main file missing) -> `git mv` preservando histórico
- pasta de build com `%TEMP%` literal dentro do repo -> removida; scripts usam TEMP
- PowerShell 5.1 lê .ps1 sem BOM como ANSI -> scripts mantidos em ASCII puro

**Arquivos:**
- `README.md`
- `docs/WIND_FOLLOW_BLIND_DOCUMENTACAO.md`
- `firmware/irobot/DashboardService.cpp`
- `firmware/irobot/HardwareConfig.h`
- `firmware/irobot/README.md`
- `firmware/windfollowblind/DashboardServiceBlind.cpp`
- `firmware/windfollowblind/windflollowblind.ino -> firmware/windfollowblind/windfollowblind.ino`
- `AGENTS.md`
- `docs/DEVLOG.md`
- `tools/flash.ps1`
- `tools/save.ps1`
