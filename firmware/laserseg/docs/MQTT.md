# MQTT local e tunel publico

**Atualizado em:** 2026-11-19  
**Escopo:** bancada do `laserseg`. Nada aqui e firmware; e infraestrutura de
mensageria da estacao de trabalho.

## O que esta rodando

| Componente | Onde | Porta | Exposicao |
|---|---|---|---|
| Mosquitto 2.1.4 | `%LOCALAPPDATA%\Programs\mosquitto` | `1883` TCP | loopback (`127.0.0.1`) |
| Mosquitto 2.1.4 | idem | `9001` WebSocket | loopback + Cloudflare Quick Tunnel |
| cloudflared 2026.11.1 | `%LOCALAPPDATA%\Programs\cloudflared` | saida HTTPS | URL `*.trycloudflare.com` |

O Mosquitto foi extraido do instalador oficial com 7-Zip, sem elevacao, e roda
como processo do usuario. Nao existe servico do Windows: a inicializacao no
logon e feita por um `.cmd` na pasta Startup.

## Autenticacao

- Usuario: `laserseg`
- Senha: gerada aleatoriamente, **somente** em
  `%USERPROFILE%\.config\opencode\secrets\mqtt-local.txt`
  (ACL restrita ao usuario atual).
- `allow_anonymous false` e `password_file` global: os dois listeners exigem
  usuario e senha.

Nunca grave a senha em arquivo do repositorio, issue ou chat.

## Operacao

```powershell
# sobe o broker + tunel e imprime a URL wss://
powershell -ExecutionPolicy Bypass -File tools\mqtt\start.ps1

# sobe apenas o broker (sem exposicao publica)
powershell -ExecutionPolicy Bypass -File tools\mqtt\start.ps1 -NoTunnel

# reinicia tudo e gera nova URL
powershell -ExecutionPolicy Bypass -File tools\mqtt\start.ps1 -Force

# estado atual (processos, portas, URL, ultimas linhas do log)
powershell -ExecutionPolicy Bypass -File tools\mqtt\status.ps1

# para broker e tunel
powershell -ExecutionPolicy Bypass -File tools\mqtt\stop.ps1
```

### Cliente

```text
TCP (local):        mqtt://127.0.0.1:1883
WebSocket (local):  ws://127.0.0.1:9001/mqtt
WebSocket (externo):wss://<subdominio>.trycloudflare.com/mqtt
```

O caminho do WebSocket e `/mqtt` (padrao do Mosquitto 2.x). O Quick Tunnel
encaminha o path sem alteracao.

## Firewall

O Quick Tunnel **nao precisa** de porta aberta: o `cloudflared` abre conexoes
de saida. Para acesso direto na LAN:

```powershell
# em PowerShell como Administrador
powershell -ExecutionPolicy Bypass -File tools\mqtt\firewall.ps1 -EnableLan
```

Isso cria a regra `Mosquitto 1883 - LAN` (perfis Domain/Private) e muda o
listener de `127.0.0.1` para `0.0.0.0`, com backup da config em
`%TEMP%\opencode\backups`. Sem `-EnableLan`, apenas a regra e criada.

O listener `9001` permanece em loopback de proposito: o tunel e o unico
caminho externo.

## Limites do Quick Tunnel

- Nao tem garantia de uptime nem SLA.
- A URL muda a cada reinicio.
- Nao tem Cloudflare Access: a unica barreira e usuario/senha do MQTT.
- Para producao, migrar para **Named Tunnel** com dominio proprio e, de
  preferencia, Cloudflare Access.

## TCP para Postgres

Quick Tunnel cobre apenas HTTP/HTTPS (incluindo WebSocket). Expor `5432` por
TCP exige **Named Tunnel** + cliente WARP/Zero Trust, nao Quick Tunnel. O
acesso do Postgres na LAN ja esta liberado no `pg_hba.conf` para faixas
RFC1918; o firewall ainda depende de uma execucao elevada.

## Validacao rapida

Teste local sem dependencias:

```powershell
mosquitto_pub -h 127.0.0.1 -p 1883 -u laserseg -P <senha> -t laserseg/teste -m ola
mosquitto_sub -h 127.0.0.1 -p 1883 -u laserseg -P <senha> -t laserseg/teste -C 1
```

Teste do WebSocket publico: conectar com `paho-mqtt`
(`transport="websockets"`, `ws_set_options(path="/mqtt")`) em
`wss://<subdominio>.trycloudflare.com` na porta `443`.
