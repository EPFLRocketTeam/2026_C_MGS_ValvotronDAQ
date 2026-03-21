#include "DAQ.h"

DAQ daq;


void setup() {
  Serial.begin(115200);
  while(!Serial) {
    // Wait for Serial to be ready
  }
  daq.setup();
  daq.setWeightOffset();
}

void loop() {
  serial_cmdHandler(&daq);
  long weight = daq.getWeight();
  Serial.print("Weight: ");
  Serial.println(weight);

  delay(100);
}