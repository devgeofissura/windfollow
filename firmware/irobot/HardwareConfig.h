#ifndef HARDWARE_CONFIG_H
#define HARDWARE_CONFIG_H

#include <Arduino.h>

// --- GPIOs do L298N (motores direito e esquerdo) ---
// Chassi diferencial: uma roda motriz por motor + uma roda caster livre na outra ponta
// Motor esquerdo (roda esquerda)
static const int8_t M_LEFT_IN1  = 26;   // Sentido 1
static const int8_t M_LEFT_IN2  = 25;   // Sentido 2
static const int8_t M_LEFT_ENA  = 27;   // PWM / habilitação (era botão 1)

// Motor direito (roda direita)
static const int8_t M_RIGHT_IN3 = 32;   // Sentido 1
static const int8_t M_RIGHT_IN4 = 33;   // Sentido 2
static const int8_t M_RIGHT_ENB = 14;   // PWM / habilitação (era botão 2)

// --- Configuração PWM ---
static const int   PWM_FREQ    = 1000;  // Hz
static const int   PWM_RES     = 8;     // bits (0-255)
static const int   PWM_CHANNEL_LEFT  = 0;
static const int   PWM_CHANNEL_RIGHT = 1;

// --- Limites de velocidade (0-255) ---
static const uint8_t PWM_MIN    = 60;   // abaixo disso o motor não gira
static const uint8_t PWM_MAX    = 255;
static const uint8_t PWM_DEFAULT = 150;

// --- Duração de pulso para movimentos temporizados (ms) ---
static const unsigned long PULSE_DURATION_DEFAULT = 500;

// --- Barramento I2C dos sensores de ambiente ---
// AHT20 (0x38) + BMP280 (0x76/0x77) na mesma placa; VL53L0X (0x29) na frente.
static const int8_t I2C_SDA = 21;
static const int8_t I2C_SCL = 22;
static const uint32_t I2C_FREQ = 100000;

static const uint8_t AHT20_I2C_ADDR  = 0x38;
static const uint8_t VL53L0X_I2C_ADDR = 0x29;

// --- Navegacao autonoma (modo captura de ambiente) ---
static const uint16_t AUTO_OBSTACLE_MM   = 250;  // abaixo disso considera obstaculo
static const uint16_t AUTO_AVOID_STOP_MS = 250;  // pausa antes de girar
static const uint16_t AUTO_TURN_MS       = 650;  // giro para desviar (roda parada como pivo)
static const uint16_t AUTO_SETTLE_MS     = 350;  // assenta antes de medir de novo
static const uint8_t  AUTO_MAX_TURNS     = 3;    // giros seguidos antes de desistir e parar
static const uint8_t  AUTO_FORWARD_SPEED = 140;  // PWM usado no avanco autonomo

// --- Estrutura de configuração salva em NVS ---
struct RobotConfig {
  int8_t  gpioLeftIn1  = M_LEFT_IN1;
  int8_t  gpioLeftIn2  = M_LEFT_IN2;
  int8_t  gpioLeftEna  = M_LEFT_ENA;
  int8_t  gpioRightIn3 = M_RIGHT_IN3;
  int8_t  gpioRightIn4 = M_RIGHT_IN4;
  int8_t  gpioRightEnb = M_RIGHT_ENB;
  uint8_t pwmMin       = PWM_MIN;
  uint8_t pwmMax       = PWM_MAX;
  uint8_t pwmDefault   = PWM_DEFAULT;
  unsigned long pulseDurationMs = PULSE_DURATION_DEFAULT;
  bool    motorsEnabled = true;
};

// --- Comandos de movimento diferencial ---
enum class RobotDirection {
  STOP,
  FRENTE,
  TRAS,
  ESQUERDA,
  DIREITA
};

#endif // HARDWARE_CONFIG_H
