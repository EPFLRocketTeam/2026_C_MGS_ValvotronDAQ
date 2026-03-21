#include <Arduino.h>

#define white 12
#define purple 13

void setup() {
  pinMode(white, OUTPUT);
  pinMode(purple, OUTPUT);
  digitalWrite(white, HIGH);
  digitalWrite(purple, HIGH);
}

void loop() {}