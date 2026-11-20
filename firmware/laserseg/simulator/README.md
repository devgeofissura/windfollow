# Simulador Web do laserseg (V1)

Página local para exercitar a interface do laserseg **sem nenhuma placa**:
estados `SAFE/TRACK/MANUAL`, telemetria `ACK`, comandos `TRACK/LIGHT/MANUAL`,
watchdog de link e provisionamento Wi-Fi (AP → STA).

> **Isto é uma simulação.** Nenhum GPIO, motor ou luz real é acionado. Os comandos
> ficam neste navegador. O firmware validado mantém `LASERSEG_ENABLE_GPIO = 0`.

## Como abrir

Duplo clique em `index.html` — funciona em `file://`, sem servidor, sem build e
sem conexão. Não é preciso instalar nada.

## O que simula

- **Aba Painel:** câmera (webcam opcional; alvo sintético por padrão),
  crosshair, estado da controladora (x/y/seq, link, luz, Wi-Fi, falha), botões
  de rastreamento, luz, perda de link, reinício e movimento manual, além do log
  de mensagens `T`/`ACK`.
- **Aba Wi-Fi:** AP público `laserseg` / `123456` em `192.168.4.1`, varredura
  simulada, campo de senha da rede local, IP da controladora (padrão
  `192.168.4.2`), fluxo salvar/reiniciar e fallback "AP e para".
- **Aba Protocolo:** constantes e formatos do contrato UDP v0.1, somente leitura.

## Regras de segurança

- A senha do AP `123456` é pública por decisão aprovada em 2026-11-19.
- A senha da rede local fica **apenas na memória da página**: não vai para log,
  arquivo, `localStorage` nem serial. O campo começa vazio.
- As constantes exibidas espelham `controller/parser.h` e
  `controller_dm556d/driver_config.h`; um teste host barra qualquer divergência.

## Validação

```powershell
# Suite host (parser + movimento + simulador):
powershell -ExecutionPolicy Bypass -File firmware/laserseg/tests/run.ps1

# Sintaxe do JavaScript:
node --check firmware/laserseg/simulator/js/app.js
```

## Limitações (fora do V1)

- Não integra com firmware real, GPIO, UDP real nem gravação na placa.
- Não substitui a bancada: valida apenas a experiência e o contrato.
- Webcam depende de permissão do navegador; sem ela, o alvo sintético continua.
- Injeção de mensagens inválidas e testes E2E de navegador ficam para o V2.

Especificação: `firmware/laserseg/spec/SPEC-simulador-web.md`.
