# MSA — Módulo de Sensores Ambientais

Firmware ESP32 para monitoramento de temperatura, umidade e pressão atmosférica,
com dashboard embarcado, API REST, alertas e configuração persistida em NVS.
A estrutura segue o mesmo padrão do firmware `irobot`: `*.ino` com setup/loop,
serviços em arquivos separados, `WebAssets.h` gerado a partir de `web/` e
detecção de porta COM por `tools/flash.ps1`.

## Hardware

| Item | Especificação |
|---|---|
| Placa | ESP32 DevKit clássico, 4 MB de flash |
| Sensor de temperatura/umidade | AHT20, endereço I2C `0x38` |
| Sensor de pressão | BMP280, endereço I2C `0x76` ou `0x77` |
| Barramento | I2C, `SDA = GPIO21`, `SCL = GPIO22`, 100 kHz |
| Alimentação dos sensores | 3,3 V e GND comum com o ESP32 |

Ligação:

```text
ESP32 3V3  -> VIN dos módulos AHT20 e BMP280
ESP32 GND  -> GND dos módulos
ESP32 GPIO21 (SDA) -> SDA dos módulos
ESP32 GPIO22 (SCL) -> SCL dos módulos
```

Não alimentar os módulos com 5 V: os pinos I2C do ESP32 não toleram 5 V.
Módulos que já trazem resistores pull-up não precisam de resistores externos.

## Leituras e alertas

- Cadência padrão: uma leitura a cada 2 s, configurável de 500 ms a 60 s.
- Temperatura exibida: AHT20, com fallback para BMP280 se o AHT20 faltar.
- A BMP280 fornece também pressão em hPa e sua própria temperatura.
- Leituras fora da faixa física são descartadas antes de entrar no dashboard.

Avisos calculados em `EnvironmentAdvisor`:

| Aviso | Regra |
|---|---|
| Calor | temperatura acima do limite configurado (padrão 26 °C) |
| Frio | temperatura abaixo do limite configurado (padrão 17 °C) |
| Ar seco | umidade abaixo do limite configurado (padrão 30%) |
| Chuva provável | pressão caindo e umidade alta, usando histórico de 30 min/3 h |
| Incêndio | temperatura ≥ 45 °C ou subida ≥ 4 °C em 1 min |

Os avisos de conforto têm histerese para não piscar no limite. O histórico de
pressão guarda 180 amostras (1/min) e o de temperatura 12 amostras (1/5 s).

## Rede

- AP padrão: `MSA-Setup` / `msa1234` — IP `192.168.4.1`.
- STA: configure SSID/senha em `/api/wifi` ou na aba Config do dashboard;
  se a conexão falhar, o firmware volta para AP.
- mDNS: `http://msa.local` quando o roteador suporta mDNS.

## API REST

| Rota | Método | Descrição |
|---|---|---|
| `/api/sensors` | GET | leituras, estado dos sensores, avisos e tendências |
| `/api/config` | GET/POST | intervalo de leitura e limites de alerta |
| `/api/system/info` | GET | uptime, heap e clientes Wi-Fi |
| `/api/wifi` | GET/POST | modo, SSID e senha salvos |
| `/api/system/restart` | POST | reinicia o ESP32 |

Exemplo de resposta de `/api/sensors`:

```json
{
  "aht": true,
  "bmp": true,
  "temperature": 24.1,
  "humidity": 86.7,
  "pressure": 945.9,
  "bmpTemperature": 24.3,
  "ageMs": 812,
  "uptime": 48213,
  "alerts": {
    "trendReady": false,
    "trend3h": 0.0,
    "trend30m": 0.0,
    "tempRise": 0.07,
    "rain": false,
    "hot": false,
    "cold": false,
    "dry": false,
    "fire": false
  }
}
```

Exemplo de POST em `/api/config`:

```json
{
  "readIntervalMs": 2000,
  "hotC": 26,
  "coldC": 17,
  "dryHumidityPct": 30,
  "rainHumidityPct": 65
}
```

Corpos POST são lidos de `arg("plain")` (o `WebServer` já bufferiza o corpo).
Todas as rotas `/api/` respondem com CORS e aceitam `OPTIONS`.

## Comandos seriais

Abrir monitor serial a 115200 baud:

| Tecla | Ação |
|---|---|
| `m` | imprime leituras e avisos atuais |
| `i` | imprime estado dos sensores e endereços I2C encontrados |
| `r` | reinicia o ESP32 |

## Compilar e gravar

Sempre detecte a porta na hora; a porta COM muda quando o cabo é replugado:

```text
powershell -ExecutionPolicy Bypass -File tools/flash.ps1 -Sketch msa -DetectOnly
powershell -ExecutionPolicy Bypass -File tools/flash.ps1 -Sketch msa
```

Se a detecção listar mais de uma porta serial, informe `-Port` com a porta do
ESP32 indicada pela detecção atual. O script confere o chip com esptool e bloqueia
a gravação se a placa não for um ESP32 clássico.

O `WebAssets.h` é gerado. Após editar qualquer arquivo em `web/`, rode a partir
de `firmware/msa`:

```text
powershell -ExecutionPolicy Bypass -File tools/generate_web_assets.ps1
```

## Arquivos

| Arquivo | Papel |
|---|---|
| `msa.ino` | setup/loop, Wi-Fi AP/STA, mDNS, comandos seriais |
| `HardwareConfig.h` | pinos I2C, limites e estrutura `MsaConfig` |
| `ConfigManager.*` | persistência NVS da configuração |
| `SensorService.*` | leitura e validação de AHT20 + BMP280 |
| `EnvironmentAdvisor.*` | histórico, tendências e avisos |
| `DashboardService.*` | servidor web, rotas REST e assets |
| `WebAssets.h` | gerado a partir de `web/` |
| `web/` | HTML, CSS, JS, manifesto e ícones da interface |
| `tools/generate_web_assets.ps1` | gera `WebAssets.h` |
