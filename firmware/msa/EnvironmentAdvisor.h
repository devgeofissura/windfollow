#ifndef ENVIRONMENT_ADVISOR_H
#define ENVIRONMENT_ADVISOR_H

#include <Arduino.h>
#include "HardwareConfig.h"
#include "SensorService.h"

struct EnvAlerts {
  bool  trendReady  = false;
  float trend3hHpa  = 0.0f;
  float trend30mHpa = 0.0f;
  float tempRiseC   = 0.0f;
  bool  rainLikely  = false;
  bool  hot         = false;
  bool  cold        = false;
  bool  dry         = false;
  bool  fire        = false;
};

class EnvironmentAdvisor {
public:
  void begin(SensorService* sensors, const MsaConfig* cfg);
  void update();
  EnvAlerts alerts() const { return _alerts; }

private:
  static const uint8_t TEMP_SAMPLES = 12;

  SensorService* _sensors = nullptr;
  const MsaConfig* _cfg = nullptr;
  EnvAlerts _alerts;

  float    _pressureHist[ENV_TREND_MINUTES];
  uint8_t  _pressureCount = 0;
  uint8_t  _pressureHead = 0;
  uint32_t _lastTrendMs = 0;

  float    _tempHist[TEMP_SAMPLES];
  uint8_t  _tempCount = 0;
  uint8_t  _tempHead = 0;
  uint32_t _lastTempMs = 0;

  void samplePressure(float hpa, uint32_t now);
  void sampleTemperature(float tempC, uint32_t now);
  void evaluate(const EnvironmentReading& r);
};

#endif
