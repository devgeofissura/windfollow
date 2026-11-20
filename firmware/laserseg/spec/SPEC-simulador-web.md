# Spec: Simulador Web do laserseg — V1

Status: proposta em 2026-11-20 — aguarda aprovacao humana antes da implementacao.

## Objective

Entregar uma pagina web local que permita ao operador **testar a interface do
laserseg sem nenhuma placa**:Painel com camera/alvo simulados, estados
`SAFE/TRACK/MANUAL`, telemetria `ACK`, comandos `TRACK/LIGHT/MANUAL`, watchdog
de link e uma tela de provisionamento Wi-Fi conforme a spec ja aprovada.

Usuario: operador de bancada que quer validar o fluxo e o vocabulario da interface
antes de existirem `camera_node/` de producao e UDP na controladora (T6).
Sucesso: abrir `index.html` no navegador e exercitar os fluxos completos sem
tocar em firmware, GPIO, motor ou luz.

### Premissas (corrigir agora se estiverem erradas)

1. Arquivos estaticos em `firmware/laserseg/simulator/`, abertos por duplo clique
   (`file://`), sem servidor, build ou framework.
2. Estado simulado em JavaScript puro, em memoria; webcam e opcional, com alvo
   sintetico como fallback quando nao houver camera ou permissao.
3. Constantes do simulador espelham `controller/parser.h` e
   `controller_dm556d/driver_config.h`; um teste host barra divergencia.
4. A tela de Wi-Fi segue `SPEC-wifi-provisioning.md`: AP publico `laserseg` /
   `123456` (excecao aprovada) e credencial da rede local **nunca** gravada em
   arquivo do repositorio — no simulador ela vive apenas na memoria da pagina.
5. Nenhuma alteracao em `controller/`, `controller_dm556d/`, GPIO, UDP real ou
   gravacao de placa. `LASERSEG_ENABLE_GPIO` continua `0`.
6. Interface em pt-BR, dark mode como cidadao de primeira classe, responsiva e
   acessivel (`label` nos campos, navegacao por teclado, sem `innerHTML` com
   entrada do usuario). Padrao visual reaproveitado de `firmware/windfollow/web/`.
7. Testes host em Python 3.14 LTS; os 92 testes atuais permanecem verdes.

## Tech Stack

- HTML5 + CSS3 + JavaScript (ES2017+), sem dependencias externas.
- `MediaDevices.getUserMedia` opcional para a webcam; fallback sintetico.
- Python 3.14 LTS (`%LOCALAPPDATA%\Programs\Python\Python314\python.exe`)
  para testes de contrato das constantes.
- Node 26.13.1 disponivel apenas para `node --check` (validacao de sintaxe).

## Commands

```powershell
# Abrir o simulador (duplo clique em index.html, sem servidor):
#   firmware/laserseg/simulator/index.html

# Validar sintaxe do JavaScript:
node --check firmware/laserseg/simulator/js/app.js

# Suite host (parser + movimento + simulador):
powershell -ExecutionPolicy Bypass -File firmware/laserseg/tests/run.ps1

# Commit com DEVLOG e push:
powershell -ExecutionPolicy Bypass -File tools/save.ps1 -Message "feat(laserseg): simulador web V1"
```

## Project Structure

```text
firmware/laserseg/
  simulator/
    index.html                # abas Painel, Wi-Fi e Protocolo
    css/style.css             # tokens, tema claro/escuro, responsividade
    js/app.js                 # FSM simulada, comandos, telemetria e log
    README.md                 # como abrir, o que simula, limitacoes
  spec/SPEC-simulador-web.md  # esta spec
  tests/test_simulator.py     # NOVO — contrato/constantes/seguranca da UI
  README.md                   # adiciona ponteiro para o simulador
  docs/ESTADO.md              # registra a ferramenta de simulacao
  docs/history.md             # linha do tempo
```

## Code Style

```js
// js/app.js — constantes espelhadas do firmware, verificadas por teste host.
const SIM = {
  MAX_DATAGRAM_BYTES: 96,
  LINK_TIMEOUT_MS: 1200,
  DEAD_BAND: 55,
  MANUAL_STEP: 80,
  MAX_TRAVEL: 533,
  TRACK_GAIN: 0.75,
  STEPS_PER_REV: 3200,
  AP_SSID: 'laserseg',
  AP_PASSWORD: '123456', // unica excecao de segredo, aprovada em 2026-11-19
};
```

Convencoes: `camelCase` em JS, `kebab-case` em arquivos, funcoes puras para
decisao (`nextState`, `targetFromError`) sempre que possivel, zeros estados em
`textContent` (nunca `innerHTML` com dados do formulario), sem bibliotecas.

## Testing Strategy

- `unittest` em `tests/test_simulator.py`, descoberto por `tests/run.ps1`.
- Verifica: arquivos existem; constantes JS casam com `parser.h` e
  `driver_config.h` por regex; mensagens `TRACK/LIGHT/MANUAL/T` e `ACK` estao
  presentes no codigo; AP e `laserseg`/`123456` e nenhuma credencial de rede
  real aparece nos arquivos; `index.html` tem `lang`, `viewport` e `label` nos
  campos de formulario.
- `node --check` como gate de sintaxe do JavaScript, quando Node estiver
  disponivel.
- Manual: abrir `index.html`, alternar tema, exercitar as tres abas, simular
  perda de link e conferir o log `ACK`.
- Fora desta V1: testes E2E de navegador (Playwright/Selenium).

## Boundaries

- **Always:** testes verdes antes de commit; backup antes de substituir arquivo;
  deteccao de placa antes de qualquer gravacao (regra da raiz, mesmo sem gravacao
  nesta spec); entrada no `DEVLOG.md` via `tools/save.ps1`.
- **Ask first:** mudar qualquer constante do contrato UDP v0.1; tocar firmware,
  GPIO, motor ou luz; alterar specs aprovadas; adicionar dependencia externa;
  integrar o simulador ao firmware real.
- **Never:** gravar a senha da rede local em arquivo ou log do repositorio;
  habilitar `LASERSEG_ENABLE_GPIO`; editar `WebAssets.h` maualmente; fingir que
  o simulador validou hardware — ele valida apenas a experiencia e o contrato.

## Success Criteria

- [ ] `index.html` abre por duplo clique, sem servidor e sem erro de sintaxe.
- [ ] Aba Painel: camera (webcam ou alvo sintetico), crosshair, status de
      `TRACK/LIGHT/MANUAL`, x/y/seq, watchdog e log de mensagens/ACK.
- [ ] Aba Wi-Fi: AP `laserseg`/`123456`, lista simulada de SSIDs, campo de senha,
      IP da controladora (padrao `192.168.4.2`), salvar/reiniciar e fallback
      "AP e para" — sem persistir a senha da rede em arquivo.
- [ ] Aba Protocolo: constantes e formatos `T,<seq>,<x>,<y>,<valid>`,
      `TRACK,<0|1>`, `LIGHT,<0|1>`, `MANUAL,<pan|tilt>,<-1|1>` e
      `ACK,<seq>,<state>,<fault>,<light>`, somente leitura.
- [ ] Simular perda de link (botao) => eixos parados, luz OFF, fault `LINK`.
- [ ] Constantes JS iguais as do firmware e defendidas por teste host.
- [ ] Tema claro/escuro, layout responsivo, navegacao por teclado e `label` nos
      formularios.
- [ ] Suite host verde: 92 testes atuais + novos testes do simulador.
- [ ] `git status` limpo apos commit e push, com DEVLOG atualizado.

## Out of Scope (Deferred)

- Integracao com firmware real ou gravacao na placa — depende de T6 e da auditoria
  de hardware; o simulador nao substitui bancada.
- Portar o parser C++ para WebAssembly: V1 replica a decisao em JS com teste de
  espelho; a fonte da verdade continua sendo `protocol_ref.py`/`parser.cpp`.
- Autenticacao no portal e no UDP: fora de escopo tambem na spec Wi-Fi aprovada.
- PWA/service worker: util apenas no firmware embarcado, nao na pagina local.
- Testes E2E de navegador: V2, se o simulador virar ferramenta de regressao.
- Traducao para ingles: V1 em pt-BR, como o resto da documentacao do subprojeto.

## Decisoes padrao se aprovado

- Aba Protocolo somente leitura; injecao manual de mensagens invalidas fica em V2.
- Alvo sintetico ligado por padrao; webcam substitui o alvo quando autorizada.
- Tema inicial segue a preferencia do sistema (`prefers-color-scheme`), com
  botao para alternar.
