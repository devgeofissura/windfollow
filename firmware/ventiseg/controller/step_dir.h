// Camada de saida STEP/DIR do ventiseg - minima e isolada.
//
// Algoritmo trivial e consagrado (mesma ideia do AccelStepper, sem copiar
// codigo e sem dependencia): recomputa o intervalo do proximo pulso a cada
// chamada, da no maximo UM pulso por chamada e limita a velocidade pela
// distancia de frenagem sqrt(2*a*dist). Nao usa delay(), nao bloqueia e nao
// toca GPIO algum ate um gate explicito ser liberado.
//
// Referencia publica consultada:
//   https://www.airspayce.com/mikem/arduino/AccelStepper/classAccelStepper.html
//   ("The run() function steps the motor once if a step is due")
//
// SEGURANCA: sem callbacks (padrao) ou com gate=false, update() apenas
// atualiza a logica; nenhum pino e escrito. O firmware mantenha
// VENTISEG_ENABLE_GPIO=0 ate P1-P5 serem conferidas fisicamente.
#ifndef VENTISEG_STEP_DIR_H
#define VENTISEG_STEP_DIR_H

#include <stdint.h>

namespace ventiseg {

constexpr int32_t kFullStepsPerRev = 200;   // motor 1,8 graus (hipotese)
constexpr int32_t kStepPulseUs = 10;        // conservador; P1 em aberto
constexpr int32_t kDirSetupUs = 5;          // setup do DIR antes do STEP
constexpr int32_t kDefaultSpeed = 800;      // passos/s
constexpr int32_t kDefaultAccel = 4000;     // passos/s^2
constexpr int32_t kSpeedMax = 10000;
constexpr int32_t kAccelMax = 100000;

// Um driver de saida por eixo. Os callbacks podem ser nullptr (bancada logica).
class StepDir {
 public:
  using WritePin = void (*)(bool level);

  StepDir(WritePin write_step = nullptr, WritePin write_dir = nullptr);

  // Configuracao so em repouso: trocar micropasso com o eixo em movimento
  // mudaria a escala no meio do trajeto.
  bool setMicrostep(int value);              // 8 -> 1600, 16 -> 3200
  bool setSpeed(int32_t value);              // passos/s
  bool setAccel(int32_t value);              // passos/s^2
  bool setGate(bool value);                  // gate de SOFTWARE (runtime)

  void setTarget(int32_t target);

  // Comando de bancada `CFG,...` (plano de configuracao, fora do parser
  // validado do protocolo). Retorna a resposta pronta para a serial.
  const char* config(const char* line);

  // Chamar com micros() monotonico. Retorna 1 se emitiu um pulso, 0 senao.
  int update(int64_t now_us);

  int32_t stepsPerRev() const { return steps_per_rev_; }
  int32_t speed() const { return speed_; }
  int32_t accel() const { return accel_; }
  bool gate() const { return gate_; }
  int32_t target() const { return target_; }
  int32_t position() const { return position_; }
  int32_t direction() const { return direction_; }
  int32_t pulses() const { return pulses_; }
  bool stepLevel() const { return step_level_; }
  bool dirLevel() const { return dir_level_; }
  double velocity() const { return velocity_; }

 private:
  WritePin write_step_;
  WritePin write_dir_;
  int32_t steps_per_rev_ = kFullStepsPerRev * 16;
  int32_t speed_ = kDefaultSpeed;
  int32_t accel_ = kDefaultAccel;
  bool gate_ = false;
  int32_t target_ = 0;
  int32_t position_ = 0;
  int32_t direction_ = 0;
  int32_t pulses_ = 0;
  double velocity_ = 0.0;
  bool started_ = false;
  int64_t last_step_us_ = 0;
  int64_t last_update_us_ = 0;
  int64_t dir_ready_us_ = 0;
  int64_t pulse_until_us_ = 0;
  bool step_level_ = false;
  bool dir_level_ = false;
};

}  // namespace ventiseg

#endif  // VENTISEG_STEP_DIR_H
