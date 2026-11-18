#ifndef MSA_HARDWARE_CONFIG_H
#define MSA_HARDWARE_CONFIG_H

#include <Arduino.h>

static const char* MSA_DEVICE_NAME = "MSA";
static const char* MSA_AP_SSID     = "MSA-Setup";
static const char* MSA_AP_PASS     = "msa1234";

static const int8_t   I2C_SDA = 21;
static const int8_t   I2C_SCL = 22;
static const uint32_t I2C_FREQ = 100000;

static const uint8_t AHT20_I2C_ADDR      = 0x38;
static const uint8_t BMP280_I2C_ADDR_A   = 0x76;
static const uint8_t BMP280_I2C_ADDR_B   = 0x77;
static const uint8_t OLED_I2C_ADDR_A     = 0x3C;
static const uint8_t OLED_I2C_ADDR_B     = 0x3D;

static const uint16_t OLED_WIDTH  = 128;
static const uint16_t OLED_HEIGHT = 64;

static const uint16_t SENSOR_INTERVAL_DEFAULT_MS = 2000;
static const uint16_t SENSOR_INTERVAL_MIN_MS     = 500;
static const uint16_t SENSOR_INTERVAL_MAX_MS     = 60000;

static const float ENV_HOT_C             = 26.0f;
static const float ENV_COLD_C            = 17.0f;
static const float ENV_DRY_HUM_PCT       = 30.0f;
static const float ENV_RAIN_HUM_PCT      = 65.0f;
static const float ENV_RAIN_HUM_30M_PCT  = 70.0f;
static const float ENV_HYST_C            = 0.5f;
static const float ENV_HYST_HUM_PCT      = 3.0f;

static const float    ENV_FIRE_C            = 45.0f;
static const float    ENV_FIRE_RISE_C       = 4.0f;
static const uint32_t ENV_FIRE_WINDOW_MS    = 60000;
static const uint32_t ENV_TREND_SAMPLE_MS   = 60000;
static const uint8_t  ENV_TREND_MINUTES     = 180;
static const float    ENV_RAIN_DROP_3H_HPA  = 1.0f;
static const float    ENV_RAIN_DROP_30M_HPA = 0.6f;

static const uint16_t MSA_CONFIG_VERSION = 1;

struct MsaConfig {
  uint16_t version        = MSA_CONFIG_VERSION;
  uint16_t readIntervalMs = SENSOR_INTERVAL_DEFAULT_MS;
  float    hotC           = ENV_HOT_C;
  float    coldC          = ENV_COLD_C;
  float    dryHumidityPct = ENV_DRY_HUM_PCT;
  float    rainHumidityPct = ENV_RAIN_HUM_PCT;
};

#endif
