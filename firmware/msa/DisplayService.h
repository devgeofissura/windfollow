#ifndef DISPLAY_SERVICE_H
#define DISPLAY_SERVICE_H

#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include "HardwareConfig.h"
#include "SensorService.h"
#include "EnvironmentAdvisor.h"

class DisplayService {
public:
  bool begin(SensorService* sensors, EnvironmentAdvisor* advisor, const MsaConfig* cfg);
  void update();

  bool available() const { return _display != nullptr; }
  uint8_t address() const { return _address; }

private:
  Adafruit_SSD1306* _display = nullptr;
  SensorService* _sensors = nullptr;
  EnvironmentAdvisor* _advisor = nullptr;
  const MsaConfig* _cfg = nullptr;
  uint8_t _address = 0;
  uint32_t _lastUpdateMs = 0;

  static bool i2cAlive(uint8_t addr);
  void render();
};

#endif
