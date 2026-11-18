# Lições aprendidas — iRobot / Windfollow

Registro consolidado dos acertos, erros e soluções encontrados durante o
desenvolvimento do iRobot e dos projetos Windfollow. Serve como memória técnica
para as próximas sessões.

## 1. Hardware e gravação

| Firmware | Placa | FQBN | Chip |
|---|---|---|---|
| `irobot` | ESP32 DevKit 4 MB + L298N + 2 motores + caster | `esp32:esp32:esp32` | ESP32 clássico |
| `windfollowblind` | ESP32 DevKit 4 MB + L298N + 2 motores + 2 botões | `esp32:esp32:esp32` | ESP32 clássico |
| `windfollow` | LILYGO T-SIMCAM V1.3, 16 MB, OPI PSRAM | `esp32:esp32:esp32s3:CDCOnBoot=cdc,USBMode=hwcdc,UploadSpeed=921600,FlashSize=16M,PartitionScheme=huge_app,PSRAM=opi` | ESP32-S3 |

- A porta COM muda quando o cabo é replugado. Nunca fixar `-p COMx`.
- Antes de qualquer gravação:

```text
powershell -ExecutionPolicy Bypass -File tools/flash.ps1 -Sketch irobot -DetectOnly
```

- Última detecção do iRobot: COM9, ESP32-D0WDQ6, 4 MB, MAC
  `a4:cf:12:75:41:44`.
- Nunca gravar `windfollow` (S3) em um DevKit clássico nem o contrário.
- Artefatos de build sempre fora do repositório, em `$env:TEMP/irobot-build`.

## 2. Navegação autônoma

O `AutoNavigator` usa uma FSM:

`SCAN` → `SCAN_MEASURE` → `SCAN_TURN` → `ALIGN_BEST` → `FORWARD` / `BACKUP` /
`TURN` / `SETTLE` → `OFF`.

- `setEnabled(true)` inicia o reconhecimento de 360° antes de andar.
- Parâmetros atuais:
  - `SCAN_STEPS = 6`
  - `SCAN_STEP_DEG = 60`
  - `SCAN_TURN_MS = 420`
  - `SCAN_MEASURE_MS = 150`
  - `SCAN_SETTLE_MS = 200`
  - `SCAN_MAX_MM = 8000`
- Leitura sem alvo registra `SCAN_MAX_MM`; o robô escolhe a direção de maior
  distância e alinha antes de seguir.
- Sem encoder, o heading é estimado pelo tempo de giro. Isso é suficiente para
  escolher a direção, mas não é odometria precisa.
- Sempre aguardar `SETTLE` depois do giro antes de medir, para não ler durante a
  vibração.
- Getters usados pelo dashboard: `headingDeg()`, `bestAngleDeg()`,
  `bestDistanceMm()`, `scanDone()`, `scanTable()`, `scanStep()`.
- Após 100 obstáculos seguidos o modo autônomo desliga e sinaliza `stuck`.

## 3. UI web responsiva

O objetivo é preencher a tela como um retângulo exato, sem scroll, em celular
paisagem.

### Regras que funcionaram

- `.app` com `height: 100dvh` e `min-height: 0`.
- Ticker, header e tabs com `flex: 0 0 auto`.
- Painéis com `flex: 1 1 auto` e `min-height: 0`.
- Grids com `minmax(0, 1fr)` em colunas e linhas.
- Nunca usar `height: calc(100% - 56px)` quando a altura do chrome varia.
- Unidade base única:

```css
:root {
  --u: clamp(5px, min(1vw, 2vh), 12px);
  --gap: calc(var(--u) * 1.1);
  --pad: calc(var(--u) * 1.3);
  --radius: calc(var(--u) * 1.5);
}
```

- Fontes, botões, cards, joystick, sensores e status usam `calc(var(--u) * n)`.
- Joystick com `aspect-ratio: 1 / 1` e largura `min(100%, calc(var(--u) * 22))`.
- O raio do knob deve ser lido do DOM:

```js
j.knobRadius = j.knob.getBoundingClientRect().width / 2;
```

### Layout da Home

- Esquerda: card do joystick ocupando a altura toda.
- Centro superior: card do D-pad.
- Faixa inferior: comandos restantes (autônomo + velocidade), sensores e status.
- Avisos ambientais no centro do header, ao lado do título iRobot.
- Letreiro do topo apenas com leituras de sensores.
- Em paisagem baixa (`max-height: 420px`), uma media query compacta fontes,
  gaps e padding para o texto do D-pad não cortar.

### Validação de layout

Screenshots nem sempre são legíveis pelo modelo. O método confiável foi medir o
DOM com Chrome headless + `puppeteer-core` em pasta temporária:

- `getBoundingClientRect()` para posição e tamanho dos cards.
- `scrollHeight` vs `clientHeight` para detectar conteúdo cortado.
- `document.body.scrollHeight` vs `innerHeight` para confirmar ausência de scroll.
- Resoluções testadas: 667×375, 844×390 e 960×540.

O input `range` nativo reporta `scrollHeight > clientHeight`; isso é normal e não
indica overflow real do layout.

## 4. PWA e ícones

- Apenas SVG no manifest não basta para instalar. Gerar PNGs reais:
  - `favicon.png` 32×32
  - `icon-192.png` 192×192
  - `icon-512.png` 512×512
  - usar `purpose: "maskable"` em um dos ícones grandes.
- Manifest com `id`, `scope`, `start_url`, `display: "fullscreen"` e
  `display_override: ["fullscreen", "standalone"]`.
- Service worker deve ter o cache versionado e incluído os novos ícones.
- Navegadores só instalam PWA de verdade em HTTPS ou localhost. No AP
  `http://192.168.4.1` pode aparecer apenas “criar atalho”.
- Rotas do firmware:
  - `/favicon.ico` e `/favicon.png` → PNG via `send_P`.
  - `/icon-192.png` e `/icon-512.png` → PNG via `send_P`.
  - `/manifest.json`, `/favicon.svg`, `/sw.js`.
- Sempre enviar `Access-Control-Allow-Origin: *` nos assets quando necessário.

## 5. Geração de assets

`WebAssets.h` é gerado. Nunca editar manualmente.

- Editar `web/index.html`, `web/css/style.css`, `web/js/app.js`, `manifest.json`
  e `sw.js`.
- Regenerar a partir de `firmware/irobot`:

```text
powershell -ExecutionPolicy Bypass -File tools/generate_web_assets.ps1
```

- Textos vão em raw string literal; binários (PNG) vão como arrays de bytes.
- Erro aprendido: linhas do array PNG sem vírgula causaram
  `expected '}' before numeric constant`. A correção foi juntar as linhas com
  `",\n"` no gerador.
- Cada linha do array tem no máximo 20 bytes para manter o header legível.

## 6. Preview local

A pasta `firmware/irobot/preview/` permite validar a UI sem hardware.

- Arquivos: `index.html`, `style.css`, `app.js`, `mock.js`, `manifest.json`,
  `sw.js` e ícones.
- Usar caminhos relativos; caminhos absolutos quebram ao abrir via `file://`.
- `mock.js` intercepta `window.fetch` e responde os endpoints `/api/*`.
- Cenários:
  - `preview/index.html` → normal.
  - `?alerts=1` → chuva, calor e ar seco.
  - `?fire=1` → alerta crítico de incêndio.
  - `?stuck=1` → robô enroscado.
- O mock remove service workers registrados para não servir cache antigo.

## 7. Fluxo de trabalho e commits

- Início de sessão: detectar a placa antes de editar/gravar.
- Ao concluir uma tarefa, salvar com:

```text
powershell -ExecutionPolicy Bypass -File tools/save.ps1 `
  -Message "feat(irobot): resumo" `
  -Summary "mudanca 1; mudanca 2" `
  -Difficulty "problema -> solucao"
```

- O script insere entrada no `docs/DEVLOG.md`, faz `git add -A`, commit e push.
- Itens de `-Summary` e `-Difficulty` são separados por `;`.
- `build/` está no `.gitignore`; artefatos que entrarem por engano devem sair do
  índice com `git rm --cached`.
- Nunca commitar segredos.

## 8. Erros e correções

| Sintoma | Causa | Correção |
|---|---|---|
| `expected '}' before numeric constant` no `WebAssets.h` | linhas do array PNG sem vírgula | juntar linhas com `,\n` no gerador |
| Texto do D-pad cortado em 844×390 | botão com altura insuficiente | media query de densidade para `max-height: 420px` |
| Joystick com knob fora de escala | raio fixo em 22 px | medir `knob.getBoundingClientRect().width / 2` |
| PWA só cria atalho | falta de PNGs válidos e/ou HTTP | PNGs 192/512 + manifest fullscreen + HTTPS/localhost |
| Preview não carrega ao abrir `file://` | caminhos absolutos `/...` | caminhos relativos |
| Arquivos de build no commit | build acidentalmente versionado | `git rm --cached` + `build/` no `.gitignore` |
| Layout estoura a tela | alturas fixas e `calc(100% - 56px)` | flex + `min-height: 0` + `100dvh` |

## 9. Checklist antes de gravar

1. Detectar a placa:

```text
powershell -ExecutionPolicy Bypass -File tools/flash.ps1 -Sketch irobot -DetectOnly
```

2. Editar somente `web/` e regenerar `WebAssets.h`.
3. Compilar:

```text
arduino-cli compile --fqbn esp32:esp32:esp32 --output-dir "$env:TEMP\irobot-build" firmware/irobot
```

4. Testar o preview em 667×375, 844×390 e 960×540.
5. Salvar com `tools/save.ps1` e confirmar o push.

## 10. Decisões que funcionaram

- Varredura de 360° antes de andar, com alinhamento na direção mais livre.
- Avisos no header e leituras no letreiro, sem duplicação.
- Comandos restantes na faixa inferior, deixando o D-pad com área previsível.
- Unidade de viewport única para escalar toda a interface.
- Medição numérica do DOM em vez de confiar apenas em screenshots.
- Preview local com mock para iterar na UI sem ligar o robô.
- Commit + push + DEVLOG ao final de cada tarefa.
