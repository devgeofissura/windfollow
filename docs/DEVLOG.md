# Diário de desenvolvimento (DEVLOG)

Registro cronológico do que foi feito em cada commit, com as dificuldades
encontradas e como foram resolvidas. Cada entrada é inserida automaticamente no
topo da seção **Histórico** por `tools/save.ps1`, que commita e faz push.

Formato de cada entrada: data/hora, mensagem do commit, o que mudou, dificuldades
e soluções, arquivos tocados.

---

## Histórico

### 2026-11-15 15:51 - feat(irobot): chassi 2 rodas + caster, deteccao automatica de placa/COM e DEVLOG automatico

**Area:** irobot, windfollowblind, tooling, docs

**Feito:**
- chassi documentado como 2 motores (1 roda cada) + roda caster livre,novo tools/flash.ps1: detecta COM na hora, confere chip com esptool e grava,novo tools/save.ps1 + docs/DEVLOG.md: commit, push e diario automaticos,AGENTS.md: fluxo de inicio de sessao e regras de porta/commit,corrige windfollowblind.ino (nao compilava) e typos de mDNS do blind,remove COM fixa dos READMEs/documentacao; README raiz ganha secao irobot

**Dificuldades e solucoes:**
- arduino-cli marca clone CH340 como Unknown -> chip identificado via esptool flash-id,windflollowblind.ino nao batia com a pasta (main file missing) -> git mv preservando historico,pasta de build com %TEMP% literal dentro do repo -> removida; scripts usam TEMP,PowerShell 5.1 le .ps1 sem BOM como ANSI -> scripts mantidos em ASCII puro

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
