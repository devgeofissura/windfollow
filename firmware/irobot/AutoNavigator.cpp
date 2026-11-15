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
  if (_stuck) return "enroscado";
  switch (_state) {
    case State::FORWARD:    return "frente";
    case State::AVOID_STOP: return "parando";
    case State::TURN:       return "desviando";
    case State::SETTLE:     return "medindo";
    case State::OFF:
    default:                return "off";
  }
}

bool AutoNavigator::blockedAhead() const {
  if (!_sensors || !_sensors->tofPresent()) return false;
  uint16_t mm = _sensors->distanceMm();
  return mm > 0 && mm < AUTO_OBSTACLE_MM;
}

void AutoNavigator::startForward() {
  _forwardSince = millis();
  _engine->move(RobotDirection::FRENTE, _speed);
  enter(State::FORWARD);
}

void AutoNavigator::handleObstacle() {
  _engine->stop();
  _avoidCount++;

  if (_avoidCount >= AUTO_STUCK_ATTEMPTS) {
    _enabled = false;
    _stuck = true;
    _stuckSeq++;
    if (_sensors) _sensors->setFastDistance(false);
    Serial.print("ENROSCADO: ");
    Serial.print(_avoidCount);
    Serial.println(" obstaculos seguidos sem avanco livre. Autonomo desligado.");
    enter(State::OFF);
    return;
  }

  Serial.print("Obstaculo ");
  Serial.print(_avoidCount);
  Serial.print("/"); Serial.println(AUTO_STUCK_ATTEMPTS);
  enter(State::AVOID_STOP);
}

bool AutoNavigator::setEnabled(bool on) {
  if (!on) {
    if (_engine && _enabled) _engine->stop();
    _enabled = false;
    _stuck = false;
    _avoidCount = 0;
    _turnSide = 0;
    if (_sensors) _sensors->setFastDistance(false);
    enter(State::OFF);
    Serial.println("Autonomo OFF");
    return true;
  }

  if (_enabled) return true;
  if (!_sensors || !_sensors->tofPresent()) {
    Serial.println("Autonomo recusado: VL53L0X ausente");
    return false;
  }

  _enabled = true;
  _stuck = false;
  _avoidCount = 0;
  _turnSide = 0;
  _sensors->setFastDistance(true);
  Serial.println("Autonomo ON");
  startForward();
  return true;
}

void AutoNavigator::loop() {
  if (!_enabled || !_engine) return;

  uint32_t elapsed = millis() - _stateSince;

  switch (_state) {
    case State::FORWARD:
      if (blockedAhead()) {
        handleObstacle();
      } else if (millis() - _forwardSince >= AUTO_FREE_FORWARD_MS) {
        if (_avoidCount > 0) {
          _avoidCount = 0;
          Serial.println("Avanco livre: contador de obstaculos zerado");
        }
      }
      break;

    case State::AVOID_STOP:
      if (elapsed >= AUTO_AVOID_STOP_MS) {
        _turnDir = (_turnSide++ % 2 == 0) ? RobotDirection::ESQUERDA
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
          handleObstacle();
        } else {
          startForward();
        }
      }
      break;

    case State::OFF:
    default:
      break;
  }
}
