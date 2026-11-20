// ventiseg - no de controle: parser serial + autoteste + telemetria.
//
// SEGURANCA: este sketch NAO configura GPIO, motor, driver, relé, luz,
// fim de curso, tilt nem WiFi. Ele valida a tabela de decisao na placa antes
// de qualquer acionamento. Nenhuma porta COM fica fixa no codigo.
#include <Arduino.h>
#include <string.h>

#include "autotest.h"
#include "driver_config.h"
#include "parser.h"

static int64_t g_monotonic_ms = 0;
static unsigned long last_heartbeat_ms = 0;
static ventiseg::Session g_session;

static void printBanner() {
  Serial.println("ventiseg controller parser: boot");
  Serial.print("chip=");
  Serial.print(ESP.getChipModel());
  Serial.print(" cores=");
  Serial.print(ESP.getChipCores());
  Serial.print(" flash=");
  Serial.print(ESP.getFlashChipSize());
  Serial.print(" gpio_gate=");
  Serial.println(VENTISEG_ENABLE_GPIO);
}

static void runChecks() {
  ventiseg::AutoTestResult result = ventiseg::runAutotest(Serial);
  (void)result;
}

static void printTelemetry() {
  char telemetry[64];
  ventiseg::formatTelemetry(telemetry, sizeof(telemetry), g_session);
  Serial.println(telemetry);
}

static void handleLine(const char* line, size_t len) {
  if (len == 3 && memcmp(line, "run", 3) == 0) {
    runChecks();
    return;
  }
  if (len == 0 || len > ventiseg::kMaxDatagramBytes) {
    Serial.println("REASON,length");
    return;
  }
  const ventiseg::Result result = g_session.feed(
      reinterpret_cast<const uint8_t*>(line), len, g_monotonic_ms,
      g_monotonic_ms);
  printTelemetry();
  if (!result.accepted) {
    Serial.print("REASON,");
    Serial.println(ventiseg::reasonName(result.reason));
  }
}

void setup() {
  Serial.begin(VENTISEG_BAUD);
  delay(300);
  g_monotonic_ms = unwrapMillis(millis(), 0);
  printBanner();
  runChecks();
  printTelemetry();
  last_heartbeat_ms = millis();
}

void loop() {
  g_monotonic_ms = unwrapMillis(millis(), g_monotonic_ms);

  // Comandos ASCII terminados por CR/LF. "run" repete o autoteste; as
  // demais linhas alimentam a mesma sessao usada no autoteste.
  static char line[ventiseg::kMaxDatagramBytes + 2] = {0};
  static size_t len = 0;
  while (Serial.available() > 0) {
    const char c = static_cast<char>(Serial.read());
    if (c == '\n' || c == '\r') {
      if (len > 0 && line[len - 1] == '\r') --len;
      handleLine(line, len);
      len = 0;
    } else if (len < sizeof(line) - 1) {
      line[len++] = c;
    } else {
      // Linha maior que o protocolo: descarta sem processar.
      len = sizeof(line);
    }
  }

  if (g_session.poll(g_monotonic_ms)) {
    Serial.println("WATCHDOG,SAFE,Lnk");
    printTelemetry();
  }

  if (millis() - last_heartbeat_ms >= 5000) {
    last_heartbeat_ms = millis();
    Serial.println("ventiseg controller parser: alive");
  }
}
