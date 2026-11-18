#include "EnvironmentAdvisor.h"

void EnvironmentAdvisor::begin(SensorService* sensors, const MsaConfig* cfg) {
  _sensors = sensors;
  _cfg = cfg;
  _alerts = EnvAlerts();

  _pressureCount = 0;
  _pressureHead = 0;
  _lastTrendMs = 0;
  _tempCount = 0;
  _tempHead = 0;
  _lastTempMs = 0;
}

void EnvironmentAdvisor::update() {
  if (!_sensors) return;

  uint32_t now = millis();
  EnvironmentReading r = _sensors->reading();

  if (!isnan(r.pressureHpa) &&
      (_pressureCount == 0 || now - _lastTrendMs >= ENV_TREND_SAMPLE_MS)) {
    samplePressure(r.pressureHpa, now);
  }
  if (!isnan(r.temperatureC) &&
      (_tempCount == 0 || now - _lastTempMs >= 5000)) {
    sampleTemperature(r.temperatureC, now);
  }

  evaluate(r);
}

void EnvironmentAdvisor::samplePressure(float hpa, uint32_t now) {
  _lastTrendMs = now;
  _pressureHist[_pressureHead] = hpa;
  _pressureHead = (_pressureHead + 1) % ENV_TREND_MINUTES;
  if (_pressureCount < ENV_TREND_MINUTES) _pressureCount++;
}

void EnvironmentAdvisor::sampleTemperature(float tempC, uint32_t now) {
  _lastTempMs = now;
  _tempHist[_tempHead] = tempC;
  _tempHead = (_tempHead + 1) % TEMP_SAMPLES;
  if (_tempCount < TEMP_SAMPLES) _tempCount++;
}

void EnvironmentAdvisor::evaluate(const EnvironmentReading& r) {
  float trend3h = 0.0f;
  float trend30m = 0.0f;
  bool ready3h = false;
  bool ready30m = false;

  if (_pressureCount >= 2) {
    uint8_t newestIdx = (_pressureHead + ENV_TREND_MINUTES - 1) % ENV_TREND_MINUTES;
    float newest = _pressureHist[newestIdx];
    uint8_t oldestIdx = (_pressureHead + ENV_TREND_MINUTES - _pressureCount) % ENV_TREND_MINUTES;
    trend3h = newest - _pressureHist[oldestIdx];
    ready3h = _pressureCount >= 30;

    if (_pressureCount >= 31) {
      uint8_t idx30 = (_pressureHead + ENV_TREND_MINUTES - 31) % ENV_TREND_MINUTES;
      trend30m = newest - _pressureHist[idx30];
      ready30m = true;
    }
  }

  _alerts.trend3hHpa = trend3h;
  _alerts.trend30mHpa = trend30m;
  _alerts.trendReady = ready3h || ready30m;

  float tempRise = 0.0f;
  if (_tempCount >= 2) {
    uint8_t newestIdx = (_tempHead + TEMP_SAMPLES - 1) % TEMP_SAMPLES;
    float newest = _tempHist[newestIdx];
    float lowest = newest;
    for (uint8_t i = 0; i < _tempCount; i++) {
      if (_tempHist[i] < lowest) lowest = _tempHist[i];
    }
    tempRise = newest - lowest;
  }
  _alerts.tempRiseC = tempRise;

  float tempC = r.temperatureC;
  bool fireRaw = (!isnan(tempC) && tempC >= ENV_FIRE_C) || tempRise >= ENV_FIRE_RISE_C;
  if (fireRaw) {
    _alerts.fire = true;
  } else if (_alerts.fire && !isnan(tempC) &&
             tempC < ENV_FIRE_C - 5.0f && tempRise < 1.0f) {
    _alerts.fire = false;
  }

  float hum = r.humidityPct;
  bool humOk = !isnan(hum);
  float rainHum = _cfg ? _cfg->rainHumidityPct : ENV_RAIN_HUM_PCT;
  bool rainRaw =
      (ready3h && humOk && trend3h <= -ENV_RAIN_DROP_3H_HPA && hum >= rainHum) ||
      (ready30m && humOk && trend30m <= -ENV_RAIN_DROP_30M_HPA &&
       hum >= rainHum + 5.0f);

  if (rainRaw) {
    _alerts.rainLikely = true;
  } else if ((_alerts.trend3hHpa > -0.4f || !ready3h) &&
             (trend30m > -0.2f || !ready30m)) {
    _alerts.rainLikely = false;
  }

  float hotC = _cfg ? _cfg->hotC : ENV_HOT_C;
  float coldC = _cfg ? _cfg->coldC : ENV_COLD_C;
  float dryHum = _cfg ? _cfg->dryHumidityPct : ENV_DRY_HUM_PCT;

  if (!isnan(tempC)) {
    if (tempC > hotC) {
      _alerts.hot = true;
    } else if (tempC < hotC - ENV_HYST_C) {
      _alerts.hot = false;
    }
    if (tempC < coldC) {
      _alerts.cold = true;
    } else if (tempC > coldC + ENV_HYST_C) {
      _alerts.cold = false;
    }
  }

  if (humOk) {
    if (hum < dryHum) {
      _alerts.dry = true;
    } else if (hum > dryHum + ENV_HYST_HUM_PCT) {
      _alerts.dry = false;
    }
  }
}
