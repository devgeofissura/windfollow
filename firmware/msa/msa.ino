#include <WiFi.h>
#include <esp_system.h>
#include <ESPmDNS.h>
#include <Preferences.h>

#include "HardwareConfig.h"
#include "ConfigManager.h"
#include "DashboardService.h"
#include "SensorService.h"
#include "EnvironmentAdvisor.h"
#include "DisplayService.h"

ConfigManager configMgr;
SensorService sensorService;
EnvironmentAdvisor envAdvisor;
DisplayService display;
DashboardService dashboard;

MsaConfig cfg;

unsigned long lastHeartbeat = 0;
unsigned long lastHealth = 0;

static void printResetReason(esp_reset_reason_t reason) {
  Serial.print("Reset anterior: ");
  Serial.print(static_cast<int>(reason));
  switch (reason) {
    case ESP_RST_POWERON:   Serial.println(" (power-on)"); break;
    case ESP_RST_EXT:       Serial.println(" (reset externo)"); break;
    case ESP_RST_SW:        Serial.println(" (software)"); break;
    case ESP_RST_PANIC:     Serial.println(" (panic/crash)"); break;
    case ESP_RST_INT_WDT:   Serial.println(" (watchdog de interrupcao)"); break;
    case ESP_RST_TASK_WDT:  Serial.println(" (watchdog de task)"); break;
    case ESP_RST_WDT:       Serial.println(" (watchdog)"); break;
    case ESP_RST_BROWNOUT:  Serial.println(" (BROWNOUT - queda de tensao)"); break;
    case ESP_RST_DEEPSLEEP: Serial.println(" (deep sleep)"); break;
    default:                Serial.println(" (desconhecido)"); break;
  }
}

static void printReading() {
  EnvironmentReading r = sensorService.reading();
  Serial.print("Temp: ");
  if (isnan(r.temperatureC)) Serial.println("--"); else Serial.println(r.temperatureC, 2);
  Serial.print("Umidade: ");
  if (isnan(r.humidityPct)) Serial.println("--"); else Serial.println(r.humidityPct, 2);
  Serial.print("Pressao: ");
  if (isnan(r.pressureHpa)) Serial.println("--"); else Serial.println(r.pressureHpa, 2);
  Serial.print("Temp BMP280: ");
  if (isnan(r.bmpTemperatureC)) Serial.println("--"); else Serial.println(r.bmpTemperatureC, 2);
  Serial.print("Idade da leitura (ms): "); Serial.println(r.ageMs);

  EnvAlerts a = envAdvisor.alerts();
  Serial.print("Avisos:");
  bool none = true;
  if (a.fire) { Serial.print(" INCENDIO"); none = false; }
  if (a.rainLikely) { Serial.print(" CHUVA"); none = false; }
  if (a.hot) { Serial.print(" CALOR"); none = false; }
  if (a.cold) { Serial.print(" FRIO"); none = false; }
  if (a.dry) { Serial.print(" AR-SECO"); none = false; }
  if (none) Serial.print(" nenhum");
  Serial.print(" (dP30m="); Serial.print(a.trend30mHpa, 2);
  Serial.print(" dP3h="); Serial.print(a.trend3hHpa, 2);
  Serial.print(" dT1min="); Serial.print(a.tempRiseC, 2);
  Serial.println(")");
}

void setup() {
  Serial.begin(115200);
  Serial.println();
  esp_reset_reason_t reason = esp_reset_reason();
  printResetReason(reason);
  Serial.println("=== MSA - Modulo de Sensores Ambientais ===");
  Serial.println("I2C: SDA=21 SCL=22  AHT20=0x38  BMP280=0x76/0x77  OLED=0x3C/0x3D");
  Serial.println();

  if (configMgr.begin()) {
    cfg = configMgr.load();
    Serial.println("Configuracao carregada do NVS");
  } else {
    Serial.println("Falha ao iniciar NVS, usando defaults");
  }

  WiFi.setSleep(false);
  WiFi.setTxPower(WIFI_POWER_11dBm);

  Preferences wifiPrefs;
  wifiPrefs.begin("msa-wifi", true);
  String wifiMode = wifiPrefs.getString("mode", "ap");
  String wifiSsid = wifiPrefs.getString("ssid", "");
  String wifiPass = wifiPrefs.getString("pass", "");
  String apSsid = wifiPrefs.getString("apSsid", MSA_AP_SSID);
  String apPass = wifiPrefs.getString("apPass", MSA_AP_PASS);
  wifiPrefs.end();
  if (apSsid.length() == 0) apSsid = MSA_AP_SSID;

  bool staOk = false;
  if (wifiMode == "sta" && wifiSsid.length() > 0) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(wifiSsid.c_str(), wifiPass.c_str());
    Serial.print("Wi-Fi STA conectando: "); Serial.println(wifiSsid);
    uint32_t t0 = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - t0 < 12000) {
      delay(250);
      Serial.print(".");
    }
    Serial.println();
    staOk = WiFi.status() == WL_CONNECTED;
    if (staOk) {
      Serial.print("Wi-Fi conectado. IP: "); Serial.println(WiFi.localIP());
    } else {
      Serial.println("Falha ao conectar no Wi-Fi - voltando para AP");
    }
  }

  if (!staOk) {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(apSsid.c_str(), apPass.c_str());
    Serial.print("AP SSID: "); Serial.println(apSsid);
    Serial.print("AP IP: "); Serial.println(WiFi.softAPIP());
  }

  if (!MDNS.begin("msa")) {
    Serial.println("Falha ao iniciar mDNS");
  } else {
    MDNS.addService("http", "tcp", 80);
    Serial.println("mDNS: http://msa.local");
  }

  if (!sensorService.begin(&cfg)) {
    Serial.println("Nenhum sensor I2C encontrado");
  } else {
    Serial.println("Sensores I2C inicializados");
  }

  envAdvisor.begin(&sensorService, &cfg);

  if (!display.begin(&sensorService, &envAdvisor, &cfg)) {
    Serial.println("Display OLED nao inicializado");
  }

  if (!dashboard.begin(&configMgr, &sensorService, &envAdvisor, &cfg, &display)) {
    Serial.println("Falha ao iniciar DashboardService!");
  } else {
    Serial.println("DashboardService ativo na porta 80");
  }

  Serial.println();
  Serial.println("Comandos seriais:");
  Serial.println("  m = ler ambiente (temp/umid/pressao/avisos)");
  Serial.println("  i = status e dispositivos I2C");
  Serial.println("  r = reiniciar");
  Serial.println();
}

void loop() {
  dashboard.loop();
  sensorService.update();
  envAdvisor.update();
  display.update();

  if (Serial.available()) {
    char cmd = Serial.read();
    switch (cmd) {
      case 'm':
        printReading();
        break;
      case 'i': {
        EnvironmentReading r = sensorService.reading();
        Serial.print("AHT20: "); Serial.println(r.ahtOk ? "ok" : "ausente");
        Serial.print("BMP280: "); Serial.println(r.bmpOk ? "ok" : "ausente");
        Serial.print("Intervalo (ms): "); Serial.println(cfg.readIntervalMs);
        uint8_t found[16];
        uint8_t count = sensorService.scan(found, sizeof(found));
        Serial.print("I2C encontrados:");
        for (uint8_t i = 0; i < count; i++) {
          Serial.print(" 0x");
          if (found[i] < 16) Serial.print('0');
          Serial.print(found[i], HEX);
        }
        Serial.println();
        Serial.print("OLED: ");
        if (display.available()) {
          Serial.print("ok em 0x");
          if (display.address() < 16) Serial.print('0');
          Serial.println(display.address(), HEX);
        } else {
          Serial.println("ausente");
        }
        break;
      }
      case 'r':
        Serial.println("Reiniciando...");
        delay(100);
        ESP.restart();
        break;
    }
  }

  unsigned long now = millis();
  if (now - lastHeartbeat > 2000) {
    lastHeartbeat = now;
    Serial.print(".");
    if (now - lastHealth > 10000) {
      lastHealth = now;
      Serial.println();
      Serial.print("[saude] heap="); Serial.print(ESP.getFreeHeap());
      Serial.print(" minHeap="); Serial.print(ESP.getMinFreeHeap());
      Serial.print(" clientesWiFi=");
      Serial.println(WiFi.getMode() == WIFI_AP ? WiFi.softAPgetStationNum() : 0);
    }
  }
}
