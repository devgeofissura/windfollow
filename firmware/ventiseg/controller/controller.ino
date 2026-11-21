// ventiseg - no de controle: parser serial + autoteste + telemetria +
// camada de saida STEP/DIR (isolada, desabilitada por padrao).
//
// SEGURANCA: com VENTISEG_ENABLE_GPIO=0 ou VENTISEG_HW_AUDIT_OK=0 este sketch
// NAO configura GPIO, motor, driver, relé, luz, fim de curso, tilt nem WiFi.
// A configuracao pela web so altera a logica; o gate fisico exige os dois
// gates de compilacao fechados apos auditoria eletrica (P1-P5).
// Nenhuma porta COM fica fixa no codigo.
#include <Arduino.h>
#include <string.h>

#include "autotest.h"
#include "driver_config.h"
#include "parser.h"
#include "step_dir.h"

static int64_t g_monotonic_ms = 0;
static int64_t g_monotonic_us = 0;
static unsigned long last_heartbeat_ms = 0;
static ventiseg::Session g_session;

#if VENTISEG_ENABLE_GPIO && VENTISEG_HW_AUDIT_OK
static void writeStepPin(bool level) {
  digitalWrite(VENTISEG_STEP_GPIO, level ? HIGH : LOW);
}

static void writeDirPin(bool level) {
  digitalWrite(VENTISEG_DIR_GPIO, level ? HIGH : LOW);
}

// Callbacks reais so existem quando os dois gates de compilacao estao fechados.
static ventiseg::StepDir g_stepDir(writeStepPin, writeDirPin);
#else
// Bancada logica: nenhum callback, nenhum pino e tocado.
static ventiseg::StepDir g_stepDir;
#endif

static void printBanner() {
  Serial.println("ventiseg controller parser: boot");
  Serial.print("chip=");
  Serial.print(ESP.getChipModel());
  Serial.print(" cores=");
  Serial.print(ESP.getChipCores());
  Serial.print(" flash=");
  Serial.print(ESP.getFlashChipSize());
  Serial.print(" gpio_gate=");
  Serial.print(VENTISEG_ENABLE_GPIO);
  Serial.print(" hw_audit=");
  Serial.println(VENTISEG_HW_AUDIT_OK);
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

// `CFG,...` e um plano de bancada para a saida: NAO passa pelo parser validado
// do protocolo v0.1 e NAO altera estado/fault/sequencia da sessao. A liberacao
// do gate exige a confirmacao explicita `CFG,GATE,1,CONFIRM`.
static void handleConfig(const char* line, size_t len) {
  char command[ventiseg::kMaxDatagramBytes + 1];
  if (len > sizeof(command) - 1) {
    Serial.println("REASON,length");
    return;
  }
  memcpy(command, line, len);
  command[len] = '\0';

#if !(VENTISEG_ENABLE_GPIO && VENTISEG_HW_AUDIT_OK)
  // Gate fechado por compilacao: aceita consulta e desligamento, recusa
  // qualquer liberacao de saida fisica mesmo que a web envie CONFIRM.
  if (strncmp(command, "CFG,GATE,1", 10) == 0) {
    Serial.println("REASON,gate");
    return;
  }
#endif

  Serial.println(g_stepDir.config(command));
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
  if (len >= 4 && memcmp(line, "CFG,", 4) == 0) {
    handleConfig(line, len);
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
  g_monotonic_us = unwrapMicros(micros(), 0);
#if VENTISEG_ENABLE_GPIO && VENTISEG_HW_AUDIT_OK
  pinMode(VENTISEG_STEP_GPIO, OUTPUT);
  pinMode(VENTISEG_DIR_GPIO, OUTPUT);
  digitalWrite(VENTISEG_STEP_GPIO, LOW);
  digitalWrite(VENTISEG_DIR_GPIO, LOW);
#endif
  printBanner();
  runChecks();
  printTelemetry();
  last_heartbeat_ms = millis();
}

void loop() {
  g_monotonic_ms = unwrapMillis(millis(), g_monotonic_ms);
  g_monotonic_us = unwrapMicros(micros(), g_monotonic_us);

  // Comandos ASCII terminados por CR/LF. "run" repete o autoteste; "CFG,"
  // configura a saida; as demais linhas alimentam a mesma sessao do autoteste.
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
    // Watchdog: parar a saida tambem, nao apenas a FSM.
    g_stepDir.setGate(false);
    g_stepDir.setTarget(g_stepDir.position());
    Serial.println("WATCHDOG,SAFE,Lnk");
    printTelemetry();
  }

  // O eixo comandado pelo parser e o alvo da camada de saida. Mesmo com gate
  // aberto, update() nao escreve pino enquanto o gate de compilacao estiver
  // fechado (callbacks nullptr).
  g_stepDir.setTarget(g_session.axis());
  g_stepDir.update(g_monotonic_us);

  if (millis() - last_heartbeat_ms >= 5000) {
    last_heartbeat_ms = millis();
    Serial.println("ventiseg controller parser: alive");
  }
}
