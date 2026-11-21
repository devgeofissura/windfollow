#include "step_dir.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

namespace ventiseg {

namespace {

// Decimal estrito no final do comando: sem sinal, espacos ou lixo.
bool parseTailInt(const char* text, int32_t* out) {
  if (text == nullptr || *text == '\0') return false;
  int64_t value = 0;
  for (const char* p = text; *p != '\0'; ++p) {
    if (*p < '0' || *p > '9') return false;
    value = value * 10 + (*p - '0');
    if (value > 1000000) return false;
  }
  *out = static_cast<int32_t>(value);
  return true;
}

}  // namespace

StepDir::StepDir(WritePin write_step, WritePin write_dir)
    : write_step_(write_step), write_dir_(write_dir) {}

bool StepDir::setMicrostep(int value) {
  if (value != 8 && value != 16) return false;
  if (velocity_ != 0.0 || step_level_) return false;  // so em repouso
  steps_per_rev_ = kFullStepsPerRev * value;
  return true;
}

bool StepDir::setSpeed(int32_t value) {
  if (value < 1 || value > kSpeedMax) return false;
  speed_ = value;
  return true;
}

bool StepDir::setAccel(int32_t value) {
  if (value < 1 || value > kAccelMax) return false;
  accel_ = value;
  return true;
}

bool StepDir::setGate(bool value) {
  if (gate_ == value) return true;
  gate_ = value;
  if (!gate_) {
    velocity_ = 0.0;
    target_ = position_;  // desabilitar estaciona; nao retoma movimento antigo
    if (step_level_) {
      step_level_ = false;
      if (write_step_) write_step_(false);
    }
  }
  return true;
}

void StepDir::setTarget(int32_t target) {
  if (target == target_) return;
  target_ = target;  // nao zerar velocidade: AccelStepper moveTo() faz o mesmo
}

const char* StepDir::config(const char* line) {
  static char reply[96];
  if (line == nullptr) return "REASON,format";

  if (strcmp(line, "CFG,?") == 0) {
    snprintf(reply, sizeof(reply),
             "CFG,steps=%d,speed=%d,accel=%d,gate=%d",
             steps_per_rev_, speed_, accel_, gate_ ? 1 : 0);
    return reply;
  }
  if (strcmp(line, "CFG,GATE,0") == 0) {
    setGate(false);
    return "CFG,GATE,0,OK";
  }
  if (strcmp(line, "CFG,GATE,1,CONFIRM") == 0) {
    setGate(true);
    return "CFG,GATE,1,OK";
  }
  if (strncmp(line, "CFG,GATE,", 9) == 0) return "REASON,format";

  int32_t value = 0;
  if (strncmp(line, "CFG,MICRO,", 10) == 0) {
    if (!parseTailInt(line + 10, &value)) return "REASON,format";
    if (value != 8 && value != 16) return "REASON,range";
    if (!setMicrostep(value)) return "REASON,busy";
    snprintf(reply, sizeof(reply), "CFG,MICRO,%d,OK", value);
    return reply;
  }
  if (strncmp(line, "CFG,SPEED,", 10) == 0) {
    if (!parseTailInt(line + 10, &value)) return "REASON,format";
    if (!setSpeed(value)) return "REASON,range";
    snprintf(reply, sizeof(reply), "CFG,SPEED,%d,OK", value);
    return reply;
  }
  if (strncmp(line, "CFG,ACCEL,", 10) == 0) {
    if (!parseTailInt(line + 10, &value)) return "REASON,format";
    if (!setAccel(value)) return "REASON,range";
    snprintf(reply, sizeof(reply), "CFG,ACCEL,%d,OK", value);
    return reply;
  }
  return "REASON,format";
}

int StepDir::update(int64_t now_us) {
  if (!started_) {
    started_ = true;
    last_update_us_ = now_us;
    last_step_us_ = now_us;
  }

  if (step_level_ && now_us >= pulse_until_us_) {
    step_level_ = false;
    if (write_step_) write_step_(false);
  }

  if (!gate_) {
    velocity_ = 0.0;
    last_update_us_ = now_us;
    return 0;
  }

  const int64_t elapsed_us = now_us - last_update_us_;
  const double dt = elapsed_us > 0 ? static_cast<double>(elapsed_us) / 1e6 : 0.0;
  last_update_us_ = now_us;

  const int64_t remaining = static_cast<int64_t>(target_) - position_;
  if (remaining == 0) {
    velocity_ = 0.0;
    return 0;
  }

  const int32_t sign = remaining > 0 ? 1 : -1;
  if (sign != direction_) {
    direction_ = sign;
    dir_level_ = sign > 0;
    dir_ready_us_ = now_us + kDirSetupUs;
    if (write_dir_) write_dir_(dir_level_);
  }
  if (now_us < dir_ready_us_) return 0;

  const int64_t dist = remaining > 0 ? remaining : -remaining;
  const double v_limit = sqrt(2.0 * static_cast<double>(accel_) *
                              static_cast<double>(dist));
  const double wanted = sign * (static_cast<double>(speed_) < v_limit
                                    ? static_cast<double>(speed_)
                                    : v_limit);
  const double delta = static_cast<double>(accel_) * dt;
  if (wanted > velocity_) {
    velocity_ = wanted < velocity_ + delta ? wanted : velocity_ + delta;
  } else if (wanted < velocity_) {
    velocity_ = wanted > velocity_ - delta ? wanted : velocity_ - delta;
  }

  const double speed_now = fabs(velocity_);
  if (speed_now < 1e-9) return 0;
  const double interval = 1e6 / speed_now;
  if (static_cast<double>(now_us - last_step_us_) < interval) return 0;

  last_step_us_ = now_us;
  position_ += sign;
  ++pulses_;
  step_level_ = true;
  pulse_until_us_ = now_us + kStepPulseUs;
  if (write_step_) write_step_(true);
  return 1;
}

}  // namespace ventiseg
