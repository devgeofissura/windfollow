#include "DisplayService.h"
#include <WiFi.h>

static const uint32_t DISPLAY_REFRESH_MS = 1000;

bool DisplayService::i2cAlive(uint8_t addr) {
  Wire.beginTransmission(addr);
  return Wire.endTransmission() == 0;
}

bool DisplayService::begin(SensorService* sensors, EnvironmentAdvisor* advisor, const MsaConfig* cfg) {
  _sensors = sensors;
  _advisor = advisor;
  _cfg = cfg;

  const uint8_t candidates[] = { OLED_I2C_ADDR_A, OLED_I2C_ADDR_B };
  for (uint8_t i = 0; i < sizeof(candidates); i++) {
    if (i2cAlive(candidates[i])) {
      _address = candidates[i];
      break;
    }
  }

  if (_address == 0) {
    Serial.println("OLED nao encontrado no I2C (0x3C/0x3D)");
    return false;
  }

  _display = new Adafruit_SSD1306(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);
  if (!_display->begin(SSD1306_SWITCHCAPVCC, _address, false, false)) {
    Serial.println("Falha ao iniciar OLED SSD1306");
    delete _display;
    _display = nullptr;
    return false;
  }

  _display->clearDisplay();
  _display->setTextColor(SSD1306_WHITE);
  _display->setTextSize(1);
  _display->setTextWrap(false);
  _display->setCursor(0, 0);
  _display->print(F("MSA iniciando"));
  _display->display();

  Serial.print("OLED SSD1306 em 0x");
  if (_address < 16) Serial.print('0');
  Serial.println(_address, HEX);
  return true;
}

void DisplayService::update() {
  if (!_display) return;
  uint32_t now = millis();
  if (_lastUpdateMs != 0 && now - _lastUpdateMs < DISPLAY_REFRESH_MS) return;
  _lastUpdateMs = now;
  render();
}

void DisplayService::render() {
  EnvironmentReading r = _sensors ? _sensors->reading() : EnvironmentReading();
  EnvAlerts a = _advisor ? _advisor->alerts() : EnvAlerts();

  char line[24];
  String alert;

  if (a.fire) alert = F("INCENDIO");
  else if (a.rainLikely) alert = F("CHUVA PROVAVEL");
  else if (a.hot) alert = F("CALOR");
  else if (a.cold) alert = F("FRIO");
  else if (a.dry) alert = F("AR SECO");
  else alert = F("SEM AVISOS");

  _display->clearDisplay();
  _display->setTextColor(SSD1306_WHITE);
  _display->setTextWrap(false);

  _display->setTextSize(1);
  _display->setCursor(0, 0);
  _display->print(F("MSA"));
  _display->setCursor(96, 0);
  _display->print(WiFi.getMode() == WIFI_AP ? F("AP") : F("STA"));
  _display->drawFastHLine(0, 11, OLED_WIDTH, SSD1306_WHITE);

  _display->setTextSize(2);
  _display->setCursor(0, 16);
  if (isnan(r.temperatureC) || isnan(r.humidityPct)) {
    _display->print(F("--.-C --%"));
  } else {
    snprintf(line, sizeof(line), "%4.1fC %3.0f%%", r.temperatureC, r.humidityPct);
    _display->print(line);
  }

  _display->setTextSize(1);
  _display->setCursor(0, 36);
  if (isnan(r.pressureHpa)) {
    _display->print(F("P -----.- hPa"));
  } else {
    snprintf(line, sizeof(line), "P %6.1f hPa", r.pressureHpa);
    _display->print(line);
  }

  _display->setCursor(0, 47);
  snprintf(line, sizeof(line), "AHT %s  BMP %s", r.ahtOk ? "ok" : "--", r.bmpOk ? "ok" : "--");
  _display->print(line);

  _display->setCursor(0, 57);
  if (alert.length() > 21) alert = alert.substring(0, 21);
  _display->print(alert);

  _display->display();
}
