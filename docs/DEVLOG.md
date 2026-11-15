# Diário de desenvolvimento (DEVLOG)

Registro cronológico do que foi feito em cada commit, com as dificuldades
encontradas e como foram resolvidas. Cada entrada é inserida automaticamente no
topo da seção **Histórico** por `tools/save.ps1`, que commita e faz push.

Formato de cada entrada: data/hora, mensagem do commit, o que mudou, dificuldades
e soluções, arquivos tocados.

---

## Histórico

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
