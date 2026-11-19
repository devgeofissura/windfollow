// laserseg - no de controladora: parser UDP + autoteste + telemetria.
//
// SEGURANCA: este sketch NAO configura GPIO, motores, fins de curso nem luz.
// Ele existe para validar a tabela de decisao na placa antes de qualquer
// acionamento. Nenhuma porta COM fica fixa no codigo.
#include <Arduino.h>

#include "autotest.h"
#include "parser.h"

static unsigned long last_heartbeat_ms = 0;

static void printBanner() {
  Serial.println("laserseg controller parser: boot");
  Serial.print("chip=");
  Serial.print(ESP.getChipModel());
  Serial.print(" cores=");
  Serial.print(ESP.getChipCores());
  Serial.print(" flash=");
  Serial.println(ESP.getFlashChipSize());
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

  if (millis() - last_heartbeat_ms >= 5000) {
    last_heartbeat_ms = millis();
    Serial.println("laserseg controller parser: alive");
  }
}
