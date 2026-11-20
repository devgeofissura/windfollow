// Parametros de movimento do no DM556D + NEMA 23 (spec aprovada 2026-11-19).
// Espelha tools/motion_config.py: tests/test_motion.py confere os valores.
// Nenhum pino e configurado neste cabecalho. GPIO so sob LASERSEG_ENABLE_GPIO.
#ifndef LASERSEG_DRIVER_CONFIG_H
#define LASERSEG_DRIVER_CONFIG_H

#include <stdint.h>

#include "../controller/parser.h"

// 3200 pulsos/volta e VALOR PROVISORIO do prototipo (equivalente a DIP
// SW5-SW8 = OFF OFF ON ON em algumas variantes). A posicao real do DIP do
// DM556D entregue NAO FOI CONFIRMADA: ver manual da unidade antes de qualquer
// ensaio com motor. Nao alterar sem atualizar tools/motion_config.py e testes.
#define LASERSEG_STEPS_PER_REV 3200
#define LASERSEG_MAX_TRAVEL 533
#define LASERSEG_DEAD_BAND 55
#define LASERSEG_TRACK_GAIN 0.75f
#define LASERSEG_MANUAL_STEP 80
#define LASERSEG_LINK_TIMEOUT_MS 1200

// Gate de seguranca: 0 = serial-only (padrao), 1 = saidas fisicas (auditoria).
#ifndef LASERSEG_ENABLE_GPIO
#define LASERSEG_ENABLE_GPIO 0
#endif

// O nucleo do parser e independente de plataforma e recebe int64_t. O ESP32
// fornece millis() em uint32_t, que retorna a zero a cada ~49,7 dias.
// Esta funcao mantem o tempo monotonico para que o watchdog de 1200 ms nao
// seja burlado por um wrap-around.
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

// Garante que a copia C++ nao divirja do contrato ja validado.
static_assert(LASERSEG_LINK_TIMEOUT_MS == laserseg::kLinkTimeoutMs,
              "link timeout divergente do nucleo");
static_assert(LASERSEG_DEAD_BAND == laserseg::kDeadBand,
              "dead band divergente do nucleo");
static_assert(LASERSEG_MANUAL_STEP == laserseg::kManualStep,
              "passo manual divergente do nucleo");

#endif  // LASERSEG_DRIVER_CONFIG_H
