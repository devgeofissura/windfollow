# AGENTS.md — regras do repositório

## Início de sessão: detectar porta e placa sem perguntar

Quando o usuário disser **"vamos trabalhar no projeto `<x>`"** (ou equivalente:
"projeto irobot", "firmware windfollowblind", …), **não pergunte a porta COM nem
o hardware**. Rode imediatamente:

```text
powershell -ExecutionPolicy Bypass -File tools/flash.ps1 -Sketch <x> -DetectOnly
```

e responda em 1–3 linhas com: **porta**, **chip**, **flash**, **MAC** e se é
compatível com o firmware pedido. Trabalhe a partir dessa configuração e
mantenha-a como referência durante a sessão; só detecte de novo antes de gravar
(a porta pode ter mudado). Se a placa conectada for incompatível com o projeto
pedido, avise antes de qualquer edição/gravação e não grave assim mesmo.

Sem `-Sketch` (só `-DetectOnly`) o script lista todas as portas e chips — use
quando ainda não se sabe qual projeto.

## Commit e push automáticos + diário de desenvolvimento

Ao concluir qualquer tarefa (ou bloco de mudanças), **sempre** commite e faça push
sem o usuário pedir. O histórico de desenvolvimento não pode ficar só na máquina
local. Use `tools/save.ps1`, que já faz tudo:

```text
powershell -ExecutionPolicy Bypass -File tools/save.ps1 `
  -Message "feat(irobot): <resumo curto>" `
  -Summary "<o que mudou 1>; <o que mudou 2>" `
  -Difficulty "<dificuldade encontrada> -> <como resolveu>"
```

Itens de `-Summary` e `-Difficulty` são separados por `;` (o `powershell.exe -File`
não converte vírgulas em lista). Não use `;` no meio do texto de um item, senão
ele é dividido em dois.

O script:

1. recusa commitar se achar possível segredo no diff;
2. insere uma entrada no topo de `docs/DEVLOG.md` (data/hora, mensagem, o que foi
   feito, dificuldades/soluções e arquivos tocados);
3. `git add -A`, commit com a mensagem informada e `git push origin <branch atual>`.

Regras:

- `docs/DEVLOG.md` é obrigatório: a cada commit precisa de uma entrada nova com
  **dificuldades encontradas e como foram superadas** (use `-Difficulty`; escreva
  “nenhuma” só se realmente não houve).
- Nunca faça commit sem push; se o push falhar, avise o usuário com o comando
  exato para repetir (`git push origin <branch>`) e mantenha o commit local.
- Uma tarefa = um commit. Evite acumular mudanças não relacionadas no mesmo commit.
- Nunca commite segredos; o script bloqueia padrões óbvios, mas a revisão é sua.

## Porta COM: nunca fixar, sempre detectar

A porta COM do ESP32 muda quando o cabo é replugado (às vezes muda até entre duas
gravações seguidas). **Nunca use `-p COMx` fixo em comandos nem documente uma porta
como se fosse permanente.** Antes de qualquer upload:

```text
powershell -ExecutionPolicy Bypass -File tools/flash.ps1 -Sketch <firmware> -DetectOnly
```

Esse comando lista as portas seriais presentes **agora**, o chip de cada uma e a
MAC. Para compilar e gravar, omita `-DetectOnly`:

```text
powershell -ExecutionPolicy Bypass -File tools/flash.ps1 -Sketch irobot
powershell -ExecutionPolicy Bypass -File tools/flash.ps1 -Sketch msa
powershell -ExecutionPolicy Bypass -File tools/flash.ps1 -Sketch windfollowblind
powershell -ExecutionPolicy Bypass -File tools/flash.ps1 -Sketch windfollow
```

O script:

1. consulta `arduino-cli board list` na hora da gravação;
2. aborta se houver 0 ou mais de 1 porta serial (aí é preciso `-Port COMx`);
3. confere o chip real com o esptool e **bloqueia** se não casar com o firmware;
4. compila no build path do perfil e grava na porta detectada.

Se `arduino-cli board list` mostrar a placa como `Unknown`, isso é normal para
clones CH340 — o chip é identificado pelo esptool dentro do script.

## Perfis de placa (fonte da verdade)

| Firmware | Pasta | FQBN | Chip esperado | Placa |
|---|---|---|---|---|
| `irobot` | `firmware/irobot` | `esp32:esp32:esp32` | ESP32 (clássico) | ESP32 DevKit 4 MB, L298N, 2 motores + caster |
| `msa` | `firmware/msa` | `esp32:esp32:esp32` | ESP32 (clássico) | ESP32 DevKit 4 MB, AHT20 + BMP280 no I2C; OLED SSD1306 onboard em LILYGO G511 (SDA 21/D1, SCL 22/D0) |
| `windfollowblind` | `firmware/windfollowblind` | `esp32:esp32:esp32` | ESP32 (clássico) | ESP32 DevKit 4 MB, L298N, 2 motores, 2 botões |
| `windfollow` | `firmware/windfollow` | `esp32:esp32:esp32s3:CDCOnBoot=cdc,USBMode=hwcdc,UploadSpeed=921600,FlashSize=16M,PartitionScheme=huge_app,PSRAM=opi` | ESP32-S3 | LILYGO T-SIMCAM V1.3, 16 MB flash, OPI PSRAM |

Nunca grave `windfollow` (S3) em um DevKit clássico nem o contrário: os pinos e o
layout de flash não são compatíveis.

## Interface web embarcada

`WebAssets.h` é gerado; edite sempre `web/` e regenere antes de compilar:

```text
powershell -ExecutionPolicy Bypass -File tools/generate_web_assets.ps1
```

Para o irobot (script próprio, roda a partir de `firmware/irobot`):

```text
powershell -ExecutionPolicy Bypass -File firmware/irobot/tools/generate_web_assets.ps1
```

Para o msa (script próprio, roda a partir de `firmware/msa`):

```text
powershell -ExecutionPolicy Bypass -File firmware/msa/tools/generate_web_assets.ps1
```

## Artefatos

Não crie pastas de build dentro do repositório. Use `$env:TEMP` (os scripts já
fazem isso). O `.gitignore` cobre `%TEMP%/`, `.build/` e binários.
