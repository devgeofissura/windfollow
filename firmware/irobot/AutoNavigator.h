#ifndef AUTO_NAVIGATOR_H
#define AUTO_NAVIGATOR_H

#include <Arduino.h>
#include "HardwareConfig.h"
#include "MotorEngine.h"
#include "SensorService.h"

// Modo autonomo do Prototipo-67: anda para frente medindo o ambiente, para ao
// detectar obstaculo, gira para desviar e retoma o avanco.
class AutoNavigator {
public:
  enum class State : uint8_t {
    OFF,
    FORWARD,
    AVOID_STOP,
    TURN,
    SETTLE,
    GIVING_UP
  };

  bool begin(MotorEngine* engine, SensorService* sensors);
  void loop();

  bool setEnabled(bool on);
  bool enabled() const { return _enabled; }
  State state() const { return _state; }
  const char* stateName() const;

private:
  MotorEngine* _engine = nullptr;
  SensorService* _sensors = nullptr;

  bool _enabled = false;
  State _state = State::OFF;
  uint32_t _stateSince = 0;
  RobotDirection _turnDir = RobotDirection::ESQUERDA;
  uint8_t _turnCount = 0;
  uint8_t _speed = AUTO_FORWARD_SPEED;

  void enter(State s);
  bool blockedAhead() const;
};

#endif // AUTO_NAVIGATOR_H
