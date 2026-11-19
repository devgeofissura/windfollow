// Probe de toolchain do laserseg: somente serial, sem GPIO.
// Usado para comprovar FQBN, upload e boot antes de qualquer firmware de producao.
#include <Arduino.h>

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("laserseg probe: boot ok");
  Serial.printf("chip=%s cores=%d flash=%u\n",
                ESP.getChipModel(), ESP.getChipCores(), ESP.getFlashChipSize());
}

void loop() {
  Serial.println("laserseg probe: alive");
  delay(2000);
}
