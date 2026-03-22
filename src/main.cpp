#include "DAQ.h"

DAQ daq;


void setup() {
  Serial.begin(115200);
  while(!Serial) {
    // Wait for Serial to be ready
  }
  delay(100);
  Serial.println(R"( _____                _          _     _      _____         _      _____      _             __               )");
  Serial.println(R"(|  __ \              (_)        | |   | |    |_   _|       | |    |_   _|    | |           / _|              )");
  Serial.println(R"(| |  \/ ___ _ __ ___  _ ___  ___| |__ | |_     | | ___  ___| |_     | | _ __ | |_ ___ _ __| |_ __ _  ___ ___ )");
  Serial.println(R"(| | __ / _ \ '_ ` _ \| / __|/ __| '_ \| __|    | |/ _ \/ __| __|    | || '_ \| __/ _ \ '__|  _/ _` |/ __/ _ \)");
  Serial.println(R"(| |_\ \  __/ | | | | | \__ \ (__| | | | |_     | |  __/\__ \ |_    _| || | | | ||  __/ |  | || (_| | (_|  __/)");
  Serial.println(R"( \____/\___|_| |_| |_|_|___/\___|_| |_|\__|    \_/\___||___/\__|   \___/_| |_|\__\___|_|  |_| \__,_|\___\___|)");
  Serial.println();
  daq.setup();
  daq.setWeightOffset();
  daq.setPressureOffset();
}

void loop() {
  update(&daq);
  delay(10);
}