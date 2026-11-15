#include "AutoNavigator.h"

bool AutoNavigator::begin(MotorEngine* engine, SensorService* sensors) {
  _engine = engine;
  _sensors = sensors;
  return _engine != nullptr && _sensors != nullptr;
}

void AutoNavigator::enter(State s) {
  _state = s;
  _stateSince = millis();
}

const char* AutoNavigator::stateName() const {
  switch (_state) {
    case State::FORWARD:   return "frente";
    case State::AVOID_STOP: return "parando";
    case State::TURN:      return "desviando";
    case State::SETTLE:    return "medindo";
    case State::GIVING_UP: return "bloqueado";
    case State::OFF:
    default:               return "off";
  }
}

bool AutoNavigator::blockedAhead() const {
  if (!_sensors || !_sensors->tofPresent()) return false;
  uint16_t mm = _sensors->distanceMm();
  return mm > 0 && mm < AUTO_OBSTACLE_MM;
}

bool AutoNavigator::setEnabled(bool on) {
  if (on && (!_sensors || !_sensors->tofPresent())) {
    Serial.println("Autonomo recusado: VL53L0X ausente");
    return false;
  }

  if (on == _enabled) return true;

  _enabled = on;
  _turnCount = 0;

  if (_sensors) _sensors->setFastDistance(on);

  if (on) {
    Serial.println("Autonomo ON");
    _engine->move(RobotDirection::FRENTE, _speed);
    enter(State::FORWARD);
  } else {
    Serial.println("Autonomo OFF");
    _engine->stop();
    enter(State::OFF);
  }
  return true;
}

void AutoNavigator::loop() {
  if (!_enabled || !_engine) return;

  uint32_t elapsed = millis() - _stateSince;

  switch (_state) {
    case State::FORWARD:
      if (blockedAhead()) {
        _engine->stop();
        enter(State::AVOID_STOP);
      }
      break;

    case State::AVOID_STOP:
      if (elapsed >= AUTO_AVOID_STOP_MS) {
        _turnDir = (_turnCount % 2 == 0) ? RobotDirection::ESQUERDA
                                         : RobotDirection::DIREITA;
        _engine->move(_turnDir, _speed);
        enter(State::TURN);
      }
      break;

    case State::TURN:
      if (elapsed >= AUTO_TURN_MS) {
        _engine->stop();
        enter(State::SETTLE);
      }
      break;

    case State::SETTLE:
      if (elapsed >= AUTO_SETTLE_MS) {
        if (blockedAhead()) {
          _turnCount++;
          if (_turnCount >= AUTO_MAX_TURNS) {
            _engine->stop();
            enter(State::GIVING_UP);
          } else {
            enter(State::AVOID_STOP);
          }
        } else {
          _turnCount = 0;
          _engine->move(RobotDirection::FRENTE, _speed);
          enter(State::FORWARD);
        }
      }
      break;

    case State::GIVING_UP:
    case State::OFF:
    default:
      break;
  }
}
