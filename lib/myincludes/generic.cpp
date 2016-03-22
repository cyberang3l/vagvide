#include "common.h"

byte degree_symbol[8] = {
  B00110, B01001, B01001, B00110,
  B00000, B00000, B00000, B00000
};

void software_Reset() {
  asm volatile ("  jmp 0");
}

void pump_operate(bool on) {
 if(on)
  digitalWrite(PUMPRELAY_PIN, LOW);
 else
  digitalWrite(PUMPRELAY_PIN, HIGH);
}

void ssr_operate(uint8_t analog_on) {
   analogWrite(SSR_PIN, analog_on);
}
