#ifndef SENSOR_SERVICE_H
#define SENSOR_SERVICE_H

#include <Arduino.h>
#include <Wire.h>
#include "HardwareConfig.h"

struct EnvironmentReading {
  bool     ahtOk      = false;
  bool     bmpOk      = false;
  bool     tofOk      = false;
  float    temperatureC = NAN;   // AHT20 (fallback: BMP280)
  float    humidityPct  = NAN;
  float    pressureHpa  = NAN;
  float    bmpTemperatureC = NAN;
  uint16_t distanceMm   = 0;     // 0 = sem leitura valida
};

class SensorService {
public:
  bool begin();

  // Chamar no loop(): faz a leitura escalonada, sem bloquear por muito tempo.
  void update();

  EnvironmentReading reading() const { return _reading; }
  uint16_t distanceMm() const { return _reading.distanceMm; }
  uint16_t lastRawMm() const { return _lastRawMm; }
  bool tofPresent() const { return _reading.tofOk; }
  void setFastDistance(bool fast) { _tofIntervalMs = fast ? 80 : 400; }

private:
  EnvironmentReading _reading;
  bool _ahtOk = false;
  bool _bmpOk = false;
  bool _tofOk = false;
  uint32_t _lastEnvMs = 0;
  uint32_t _lastTofMs = 0;
  uint16_t _lastRawMm = 0;
  uint16_t _tofIntervalMs = 400;

  void readEnvironment();
  void readDistance();
};

#endif // SENSOR_SERVICE_H
