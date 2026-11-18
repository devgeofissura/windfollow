#ifndef SENSOR_SERVICE_H
#define SENSOR_SERVICE_H

#include <Arduino.h>
#include <Wire.h>
#include "HardwareConfig.h"

struct EnvironmentReading {
  bool     ahtOk           = false;
  bool     bmpOk           = false;
  float    temperatureC    = NAN;
  float    humidityPct     = NAN;
  float    pressureHpa     = NAN;
  float    bmpTemperatureC = NAN;
  uint32_t ageMs           = 0;
};

class SensorService {
public:
  bool begin(const MsaConfig* cfg);
  void update();

  EnvironmentReading reading() const {
    EnvironmentReading out = _reading;
    out.ageMs = millis() - _lastEnvMs;
    return out;
  }
  uint8_t scan(uint8_t* found, uint8_t capacity) const;

private:
  const MsaConfig* _cfg = nullptr;
  EnvironmentReading _reading;
  bool _ahtOk = false;
  bool _bmpOk = false;
  uint32_t _lastEnvMs = 0;

  void readEnvironment();
  static bool validTemperature(float v);
  static bool validHumidity(float v);
  static bool validPressure(float v);
};

#endif
