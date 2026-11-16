#ifndef ENVIRONMENT_ADVISOR_H
#define ENVIRONMENT_ADVISOR_H

#include <Arduino.h>
#include "HardwareConfig.h"
#include "SensorService.h"

// Avisos derivados dos sensores ambientais (AHT20 + BMP280 e, opcionalmente,
// sensor de chuva FC-37/YL-83 e sensor de chama KY-026).
struct EnvAlerts {
  bool  trendReady   = false;   // ja existe historico de pressao suficiente
  float trend3hHpa   = 0.0f;    // variacao de pressao nas ultimas 3 h
  float trend30mHpa  = 0.0f;    // variacao de pressao nos ultimos 30 min
  float tempRiseC    = 0.0f;    // subida de temperatura na janela de incendio
  bool  rainLikely   = false;   // previsao: tempo virando para chuva
  bool  rainSensor   = false;   // sensor fisico de chuva molhado (se instalado)
  bool  hot          = false;   // acima de ENV_HOT_C -> ligar ar-condicionado
  bool  cold         = false;   // abaixo de ENV_COLD_C -> desligar ar-condicionado
  bool  dry          = false;   // umidade baixa -> beber agua
  bool  fire         = false;   // possivel incendio
};

class EnvironmentAdvisor {
public:
  void begin(SensorService* sensors);
  void update();
  EnvAlerts alerts() const { return _alerts; }

private:
  static const uint8_t TEMP_SAMPLES = 12;   // 12 x 5 s = 60 s de janela

  SensorService* _sensors = nullptr;
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

#endif // ENVIRONMENT_ADVISOR_H
