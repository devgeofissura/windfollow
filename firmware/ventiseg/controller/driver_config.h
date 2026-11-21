// Parametros do eixo unico do ventiseg (pan do fluxo de ar).
// Espelha tools/protocol_ref.py e simulator/js/app.js: testes host barram
// divergencia. Nenhum pino e configurado neste cabecalho; GPIO so existe
// sob VENTISEG_ENABLE_GPIO=1 apos auditoria eletrica (P1-P5).
#ifndef VENTISEG_DRIVER_CONFIG_H
#define VENTISEG_DRIVER_CONFIG_H

#include <stdint.h>

#include "parser.h"

// 3200 pulsos/volta e VALOR PROVISORIO, conforme decidido em bancada.
// A revisao real do DM556D Azaneli NAO FOI CONFIRMADA: ver
// docs/HARDWARE-DECISIONS.md antes de qualquer ensaio com motor.
#define VENTISEG_STEPS_PER_REV 3200
#define VENTISEG_MAX_TRAVEL 533
#define VENTISEG_TRACK_GAIN 0.75f
#define VENTISEG_DEAD_BAND 55
#define VENTISEG_MANUAL_STEP 80
#define VENTISEG_LINK_TIMEOUT_MS 1200
#define VENTISEG_BAUD 115200

// Pinagem confirmada por serigrafia (J_GPIO_LIVRES). macros informativos:
// NAO sao usados enquanto o gate de seguranca estiver em zero.
#define VENTISEG_STEP_GPIO 3
#define VENTISEG_DIR_GPIO 21
// IO45 e IO46 ficam reservados por strapping - nunca usar no firmware.

// Gate de seguranca: 0 = serial-only (padrao), 1 = saidas fisicas (auditoria).
#ifndef VENTISEG_ENABLE_GPIO
#define VENTISEG_ENABLE_GPIO 0
#endif

// Segundo gate: mesmo com VENTISEG_ENABLE_GPIO=1, nenhum pino e escrito ate
// a auditoria eletrica P1-P5 fechar. Nao alterar sem evidencia fisica.
#ifndef VENTISEG_HW_AUDIT_OK
#define VENTISEG_HW_AUDIT_OK 0
#endif

// micros() do ESP32 e uint32_t e volta a zero a cada ~71,6 min. Mantem o
// relogio monotonico da camada STEP/DIR pelo mesmo principio de unwrapMillis.
constexpr int64_t unwrapMicros(uint32_t sample, int64_t previous) {
  const uint32_t last = static_cast<uint32_t>(previous);
  const uint32_t delta = sample - last;
  return previous + static_cast<int64_t>(delta);
}

static_assert(unwrapMicros(1000u, 900LL) == 1000LL,
              "incremento normal do micros divergente");
static_assert(unwrapMicros(1100u, 4294967196LL) == 4294968396LL,
              "wrap do micros divergente");

// O nucleo do parser recebe int64_t. millis() do ESP32 e uint32_t e retorna
// a zero a cada ~49,7 dias; esta funcao mantem o tempo monotonico para que
// o watchdog de 1200 ms nao seja burlado por um wrap-around.
constexpr int64_t unwrapMillis(uint32_t sample, int64_t previous) {
  const uint32_t last = static_cast<uint32_t>(previous);
  const uint32_t delta = sample - last;  // subtracao modular de uint32_t
  return previous + static_cast<int64_t>(delta);
}

// Prova em tempo de compilacao da fronteira do watchdog apos um wrap:
// 2^32-100 -> 1101 equivale a 1201 ms, acima do limite de 1200 ms.
static_assert(unwrapMillis(1000u, 900LL) == 1000LL,
              "incremento normal do millis divergente");
static_assert(unwrapMillis(1101u, 4294967196LL) == 4294968397LL,
              "wrap do millis divergente");

// Garante que a copia C++ nao divirja do contrato ja validado na host.
static_assert(VENTISEG_LINK_TIMEOUT_MS == ventiseg::kLinkTimeoutMs,
              "link timeout divergente do nucleo");
static_assert(VENTISEG_DEAD_BAND == ventiseg::kDeadBand,
              "dead band divergente do nucleo");
static_assert(VENTISEG_MANUAL_STEP == ventiseg::kManualStep,
              "passo manual divergente do nucleo");
static_assert(VENTISEG_MAX_TRAVEL == ventiseg::kMaxTravel,
              "curso maximo divergente do nucleo");
static_assert(VENTISEG_STEPS_PER_REV == ventiseg::kStepsPerRev,
              "passos por volta divergentes do nucleo");
static_assert(VENTISEG_TRACK_GAIN == ventiseg::kTrackGain,
              "ganho de tracking divergente do nucleo");

#endif  // VENTISEG_DRIVER_CONFIG_H
