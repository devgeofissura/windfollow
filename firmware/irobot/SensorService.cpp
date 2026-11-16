#include "SensorService.h"

#include <Adafruit_AHTX0.h>
#include <Adafruit_BMP280.h>
#include <VL53L0X.h>

static Adafruit_AHTX0 aht;
static Adafruit_BMP280 bmp;
static VL53L0X tof;

static const uint16_t ENV_INTERVAL_MS = 2000;

bool SensorService::begin() {
  Wire.begin(I2C_SDA, I2C_SCL, I2C_FREQ);

  _ahtOk = aht.begin(&Wire);
  if (!_ahtOk) {
    Serial.println("AHT20 nao encontrado no I2C");
  }
  _reading.ahtOk = _ahtOk;

  _bmpOk = bmp.begin(0x76) || bmp.begin(0x77);
  if (_bmpOk) {
    bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,
                    Adafruit_BMP280::SAMPLING_X2,
                    Adafruit_BMP280::SAMPLING_X16,
                    Adafruit_BMP280::FILTER_X16,
                    Adafruit_BMP280::STANDBY_MS_500);
  } else {
    Serial.println("BMP280 nao encontrado no I2C");
  }
  _reading.bmpOk = _bmpOk;

  tof.setBus(&Wire);
  tof.setTimeout(500);
  _tofOk = tof.init();
  if (_tofOk) {
    tof.setMeasurementTimingBudget(20000);
  } else {
    Serial.println("VL53L0X nao encontrado no I2C");
  }
  _reading.tofOk = _tofOk;

  return _ahtOk || _bmpOk || _tofOk;
}

void SensorService::update() {
  uint32_t now = millis();

  if (now - _lastEnvMs >= ENV_INTERVAL_MS) {
    _lastEnvMs = now;
    readEnvironment();
  }

  if (_tofOk && now - _lastTofMs >= _tofIntervalMs) {
    _lastTofMs = now;
    readDistance();
  }
}

void SensorService::readEnvironment() {
  if (_ahtOk) {
    sensors_event_t humidityEvent;
    sensors_event_t tempEvent;
    aht.getEvent(&humidityEvent, &tempEvent);
    if (!isnan(tempEvent.temperature)) {
      _reading.temperatureC = tempEvent.temperature;
    }
    if (!isnan(humidityEvent.relative_humidity)) {
      _reading.humidityPct = humidityEvent.relative_humidity;
    }
  }

  if (_bmpOk) {
    float p = bmp.readPressure();
    float t = bmp.readTemperature();
    if (p > 0) _reading.pressureHpa = p / 100.0f;
    if (!isnan(t)) _reading.bmpTemperatureC = t;
    if (!_ahtOk && !isnan(t)) _reading.temperatureC = t;
  }
}

void SensorService::readDistance() {
  uint16_t mm = tof.readRangeSingleMillimeters();
  _lastRawMm = mm;
  if (tof.timeoutOccurred()) {
    _lastRawMm = 9999;
    return;
  }
  // VL53L0X devolve ~8190 quando nao ha alvo no alcance
  if (mm >= 8000) {
    _reading.distanceMm = 0;
  } else {
    _reading.distanceMm = mm;
  }
}
