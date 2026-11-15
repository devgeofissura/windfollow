#ifndef AUTO_NAVIGATOR_H
#define AUTO_NAVIGATOR_H

#include <Arduino.h>
#include "HardwareConfig.h"
#include "MotorEngine.h"
#include "SensorService.h"

// Modo autonomo do Prototipo-67: anda sem parar, desvia de obstaculos e so
// encerra quando o usuario desliga ou quando detecta que ficou enroscado
// (AUTO_STUCK_ATTEMPTS obstaculos seguidos sem AUTO_FREE_FORWARD_MS de avanco).
class AutoNavigator {
public:
  enum class State : uint8_t {
    OFF,
    FORWARD,
    AVOID_STOP,
    TURN,
    SETTLE
  };

  bool begin(MotorEngine* engine, SensorService* sensors);
  void loop();

  bool setEnabled(bool on);
  bool enabled() const { return _enabled; }
  bool stuck() const { return _stuck; }
  uint32_t stuckSeq() const { return _stuckSeq; }
  uint16_t avoidCount() const { return _avoidCount; }
  State state() const { return _state; }
  const char* stateName() const;

private:
  MotorEngine* _engine = nullptr;
  SensorService* _sensors = nullptr;

  bool _enabled = false;
  bool _stuck = false;
  uint32_t _stuckSeq = 0;
  uint16_t _avoidCount = 0;

  State _state = State::OFF;
  uint32_t _stateSince = 0;
  uint32_t _forwardSince = 0;
  RobotDirection _turnDir = RobotDirection::ESQUERDA;
  uint8_t _turnSide = 0;
  uint8_t _speed = AUTO_FORWARD_SPEED;

  void enter(State s);
  void startForward();
  void handleObstacle();
  bool blockedAhead() const;
};

#endif // AUTO_NAVIGATOR_H
