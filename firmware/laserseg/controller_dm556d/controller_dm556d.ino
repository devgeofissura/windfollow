// laserseg - no controladora DM556D + NEMA 23 (fase serial-only).
//
// SEGURANCA: LASERSEG_ENABLE_GPIO = 0. Este sketch NAO configura GPIO,
// motores, fins de curso, luz nem Wi-Fi. Ele reusa o nucleo validado de
// controller/ e repete o autoteste na placa antes de qualquer acionamento.
// Nenhuma porta COM fica fixa no codigo.
#include <Arduino.h>

#include "driver_config.h"
#include "../controller/autotest.h"
#include "../controller/parser.h"

static unsigned long last_heartbeat_ms = 0;
static int64_t monotonic_ms = 0;

// Tempo monotonico para o watchdog: sobrevive ao wrap do millis() do ESP32.
static int64_t nowMs() {
  monotonic_ms = unwrapMillis(millis(), monotonic_ms);
  return monotonic_ms;
}

static void printBanner() {
  Serial.println("laserseg controller DM556D: boot (serial-only)");
  Serial.print("chip=");
  Serial.print(ESP.getChipModel());
  Serial.print(" cores=");
  Serial.print(ESP.getChipCores());
  Serial.print(" flash=");
  Serial.println(ESP.getFlashChipSize());
  Serial.print("gpio=");
  Serial.println(LASERSEG_ENABLE_GPIO);
  Serial.print("steps_per_rev=");
  Serial.println(LASERSEG_STEPS_PER_REV);
}

static void runChecks() {
  laserseg::AutoTestResult result = laserseg::runAutotest(Serial);
  (void)result;
}

void setup() {
  Serial.begin(115200);
  delay(300);
  printBanner();
  runChecks();
  last_heartbeat_ms = millis();
  (void)nowMs();
}

void loop() {
  // Comando opcional pela serial: "run" + Enter repete o autoteste.
  static String line;
  while (Serial.available() > 0) {
    const char c = static_cast<char>(Serial.read());
    if (c == '\n' || c == '\r') {
      line.trim();
      if (line == "run") runChecks();
      line = "";
    } else if (line.length() < 16) {
      line += c;
    }
  }

  // Mantem o relogio monotonico vivo mesmo sem trafego (watchdog do link).
  (void)nowMs();

  if (millis() - last_heartbeat_ms >= 5000) {
    last_heartbeat_ms = millis();
    Serial.println("laserseg controller DM556D: alive");
  }
}
