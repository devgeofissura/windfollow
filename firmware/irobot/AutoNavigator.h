#ifndef AUTO_NAVIGATOR_H
#define AUTO_NAVIGATOR_H

#include <Arduino.h>
#include "HardwareConfig.h"
#include "MotorEngine.h"
#include "SensorService.h"

// Modo autonomo do Prototipo-67 (roda traseira fixa, sem encoder).
//
// Ciclo de desvio, inspirado nos FSMs de obstacle avoidance para ESP32
// (stop -> backup -> pivot turn -> scan -> forward):
//   FRENTE: anda reto medindo a frente; para a 50 cm do obstaculo.
//   RE:     recua (500 ms; ate 800 ms a cada 3 obstaculos) para abrir espaco.
//   GIRANDO: pivo no proprio eixo (uma roda em re, outra em frente) e fica
//            girando ate a frente ter AUTO_CLEAR_MM (1 m) por 2 leituras.
//            Se girar demais sem achar saida, recua um tico e continua; se
//            ainda assim nao achar, troca de lado.
//   MEDINDO: assenta e confirma a clareza antes de retomar o reto.
//
// Nunca para sozinho: encerra apenas quando o usuario desliga ou quando
// acumula AUTO_STUCK_ATTEMPTS obstaculos sem AUTO_FREE_FORWARD_MS de avanco.
class AutoNavigator {
public:
  enum class State : uint8_t {
    OFF,
    FORWARD,
    BACKUP,
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
  uint16_t _backupMs = AUTO_BACKUP_MS;
  uint8_t _side = 0;           // 0 = gira pela esquerda, 1 = pela direita
  uint8_t _turnSegments = 0;   // giros consecutivos sem achar 1 m livre
  uint8_t _clearStreak = 0;

  void enter(State s);
  void startForward();
  void startTurn();
  void startBackup(uint16_t ms);
  void handleObstacle();
  bool registerFailure();
  void goStuck();
  bool blockedAhead() const;
  bool clearAhead() const;
};

#endif // AUTO_NAVIGATOR_H
