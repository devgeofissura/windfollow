---
name: udp-protocol
description: Use ao implementar, alterar, testar ou depurar o enlace UDP do laserseg entre a câmera T-SIMCAM e a controladora, incluindo mensagens TRACK, LIGHT, MANUAL e T, timeout de link e robustez de parser.
---

# Protocolo UDP — laserseg

## Contrato vigente

- UDP, porta `4210`, escuta na controladora.
- AP: câmera `192.168.4.1`, controladora `192.168.4.2`.
- Sem autenticação no protótipo: somente rede local confiável.
- Câmera → controladora; telemetria de volta ainda não definida.

| Mensagem | Formato | Efeito |
|---|---|---|
| Tracking | `TRACK,0` / `TRACK,1` | habilita/desabilita seguimento |
| Luz | `LIGHT,0` / `LIGHT,1` | desliga/liga saída do foco |
| Manual | `MANUAL,pan\|tilt,-1\|1` | passo manual de 80 passos limitado a `MAX_TRAVEL` |
| Amostra | `T,<seq>,<x>,<y>,<valid>` | erro normalizado de -1000..1000 e validade 0/1 |

## Passos

1. Leia `docs/PROTOCOLO_UDP.md` antes de tocar em qualquer parser.
2. Todo datagrama recebido deve passar por: formato → faixa numérica → sequência → idade.
   Sequência repetida, `seq` regressivo fora da janela e amostra expirada são descartados.
3. Aplique os limites localmente na controladora, mesmo que a mensagem venha de fonte
   confiável: `MAX_TRAVEL = STEPS_PER_REV / 6`, `DEAD_BAND = 55`, `LINK_TIMEOUT_MS = 1200`.
4. Em `TRACK,0`, `valid=0`, parse inválido ou perda de link: parar eixos; em perda de link
   também desligar a luz.
5. Escreva testes de parse com mensagens sintéticas **antes** de ligar motores:
   - `T,1,0,0,1` → nenhum movimento além da zona morta.
   - `T,2,1000,-1000,1` → comando dentro do limite, sentido a confirmar em ensaio.
   - `T,3,1000,0,0` → nenhum movimento.
   - `T,3,9999,0,1`, `T,abc,0,0,1`, `TRACK,7`, `MANUAL,roll,1` → rejeitados.
   - ausência de pacote por > 1200 ms → parada + luz off.
6. Se o formato mudar, atualize no mesmo commit: ambos os sketches, `PROTOCOLO_UDP.md`,
   `ESTADO.md` e `DEVLOG.md`.

## Regras

- Não transmitir vídeo por UDP; apenas posição/erro e comandos.
- O sentido de `x`/`y` ainda precisa de ensaio: não inverter sinal "por intuição".
- `TRACK_GAIN` difere entre protótipos (0.75 DRV8834, 0.55 ULN2003). Não copiar às cegas.
- Antes de operação supervisionada, prever `ack_seq`, `state` e `fault` na volta.

## Critério de saída

Parser aceita só mensagens válidas; falha sempre leva a estado seguro; testes sintéticos
registrados; nenhuma dependência de rede confiável para limites de segurança.
