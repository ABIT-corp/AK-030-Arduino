#include "AK-030.h"
#include "Arduino.h"

void setup() {
  pinMode(PIN_LED_POWER, OUTPUT);  // Left Side Blue LED
  pinMode(PIN_LED_RADIO, OUTPUT);  // Right Side Orange LED

  digitalWrite(PIN_LED_POWER, HIGH);  // turn off LED (negative logic)
  digitalWrite(PIN_LED_RADIO, HIGH);  // turn off LED (negative logic)
}
void loop() {
  digitalWrite(PIN_LED_POWER, HIGH);  // off
  digitalWrite(PIN_LED_RADIO, LOW);   // on
  delay(500);
  digitalWrite(PIN_LED_POWER, LOW);   // on
  digitalWrite(PIN_LED_RADIO, HIGH);  // off
  delay(500);
}
