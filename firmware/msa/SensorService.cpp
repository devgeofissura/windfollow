#include "SensorService.h"

#include <Adafruit_AHTX0.h>
#include <Adafruit_BMP280.h>

static Adafruit_AHTX0 aht;
static Adafruit_BMP280 bmp;

bool SensorService::begin(const MsaConfig* cfg) {
  _cfg = cfg;
  Wire.begin(I2C_SDA, I2C_SCL, I2C_FREQ);

  _ahtOk = aht.begin(&Wire);
  if (!_ahtOk) {
    Serial.println("AHT20 nao encontrado no I2C");
  }
  _reading.ahtOk = _ahtOk;

  _bmpOk = bmp.begin(BMP280_I2C_ADDR_A) || bmp.begin(BMP280_I2C_ADDR_B);
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

  return _ahtOk || _bmpOk;
}

void SensorService::update() {
  uint32_t now = millis();
  uint32_t interval = _cfg ? _cfg->readIntervalMs : SENSOR_INTERVAL_DEFAULT_MS;
  if (interval < SENSOR_INTERVAL_MIN_MS) interval = SENSOR_INTERVAL_MIN_MS;
  if (interval > SENSOR_INTERVAL_MAX_MS) interval = SENSOR_INTERVAL_MAX_MS;

  if (_lastEnvMs == 0 || now - _lastEnvMs >= interval) {
    _lastEnvMs = now;
    readEnvironment();
  }
}

void SensorService::readEnvironment() {
  if (_ahtOk) {
    sensors_event_t humidityEvent;
    sensors_event_t tempEvent;
    aht.getEvent(&humidityEvent, &tempEvent);
    if (validTemperature(tempEvent.temperature)) {
      _reading.temperatureC = tempEvent.temperature;
    }
    if (validHumidity(humidityEvent.relative_humidity)) {
      _reading.humidityPct = humidityEvent.relative_humidity;
    }
  }

  if (_bmpOk) {
    float p = bmp.readPressure();
    float t = bmp.readTemperature();
    if (validPressure(p / 100.0f)) _reading.pressureHpa = p / 100.0f;
    if (validTemperature(t)) _reading.bmpTemperatureC = t;
    if (!_ahtOk && validTemperature(t)) _reading.temperatureC = t;
  }

}

uint8_t SensorService::scan(uint8_t* found, uint8_t capacity) const {
  if (!found || capacity == 0) return 0;
  uint8_t count = 0;
  for (uint8_t addr = 1; addr < 127 && count < capacity; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      found[count++] = addr;
    }
  }
  return count;
}

bool SensorService::validTemperature(float v) {
  return !isnan(v) && v >= -40.0f && v <= 85.0f;
}

bool SensorService::validHumidity(float v) {
  return !isnan(v) && v >= 0.0f && v <= 100.0f;
}

bool SensorService::validPressure(float v) {
  return !isnan(v) && v >= 300.0f && v <= 1100.0f;
}
