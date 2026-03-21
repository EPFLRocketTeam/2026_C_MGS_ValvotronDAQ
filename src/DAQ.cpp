#include "DAQ.h"

void update(DAQ *daq) {
    serial_cmdHandler(daq);
    switch(currentState) {
        case IDLE:
            // Nothing to do
            break;
        case LOADED:
            // When fuel and igniter loaded, keep igniter off
            digitalWrite(IGNITER_PIN, LOW);
            break;
        case ARMED:
            // when ready for firing
            break;
        case FIRE:
            // firing sequence
            daq->fireSequence();
            currentState = RECOVER;
            break;
        case LOAD_CELL_VERBOSE:
            // Continuously print load cell readings until state change
            Serial.print("current weight: ");
            Serial.println(daq->getWeight());
            delay(100);
            break;
        case PRESSURE_VERBOSE:
            // Continuously print pressure sensor readings until state change
            Serial.print("current pressure: ");
            Serial.println(daq->getPressure());
            delay(100);
            break;
        default:
            // Handle unexpected state
            break;
    }
}

DAQ::DAQ() {
    weightOffset = 0;
    calibrationFactor = 1.0; // Default to 1.0 to avoid division by zero
    TCA9548 mux(MUX_ADDRESS, &Wire1); // Initialize multiplexer object
}

void DAQ::setup() {
    pinMode(IGNITER_PIN, OUTPUT);
    digitalWrite(IGNITER_PIN, LOW);
    pinMode(MUX_RESET_PIN, OUTPUT);
    digitalWrite(MUX_RESET_PIN, HIGH); // Ensure multiplexer is not in reset state
    initLoadCell();
    initPressureSensor();
}

void DAQ::initLoadCell() {
    scale.begin(DATA_PIN, CLK_PIN);
    delay(100); // Short delay to ensure HX711 is ready
    if ( scale.is_ready() ) {
        Serial.println("HX711 initialized successfully.");
    } else {
        Serial.println("Error: HX711 not detected during initialization.");
    }
}

void DAQ::initPressureSensor() {
    Wire1.end(); // Ensure Wire1 is not already initialized
    delay(100); // Short delay to ensure clean start
    Wire1.begin(); // Initialize I2C on Wire1
    resetMux(); // Reset the multiplexer to ensure it's in a known state
    if (isPressureConnected()) {
        Serial.println("Pressure sensor initialized successfully.");
    } else {
        Serial.println("Error: Pressure sensor not detected during initialization.");
    }
}

void DAQ::setWeightOffset() {
    static float weightOffsetSum = 0;
    const unsigned long timeToRead = 5000; // Time in milliseconds to read the weightOffset
    static int readingsCount = 0;
    unsigned long startTime = millis();

    Serial.println("Calibrating weightOffset... Please wait.");
    while (millis() - startTime < timeToRead) {
        if (scale.is_ready()) {
            weightOffsetSum += scale.read();
            readingsCount++;
        }
    }

    if (readingsCount > 0) {
        weightOffset = weightOffsetSum / readingsCount;
    }
    else {
        weightOffset = 0; // Default to 0 if no readings were taken
        Serial.println("Warning: No readings taken for weightOffset calibration. Defaulting to 0.");
    }

    Serial.print("weightOffset calibrated: ");
    Serial.println(weightOffset);
    Serial.println("--------------------------------------------------------------");
}

void DAQ::setPressureOffset() {
    // Similar implementation to setWeightOffset, but for pressure sensor
    static float pressureOffsetSum = 0;
    const unsigned long timeToRead = 5000; // Time in milliseconds to read the pressureOffset
    static int readingsCount = 0;
    unsigned long startTime = millis();

    Serial.println("Calibrating pressureOffset... Please wait.");
    while (millis() - startTime < timeToRead) {
        mux.selectChannel(0); // Select the multiplexer channel
        if (isPressureConnected()) { // Check if the sensor is connected
            int16_t rawPressure = readRawPressure(); // Read the digital output
            if (rawPressure != ABERRANT_DATA) {
                pressureOffsetSum += rawPressure;
                readingsCount++;
            }
        }
    }
    if (readingsCount > 0) {
        pressureOffset = pressureOffsetSum / readingsCount;
    }
    else {
        pressureOffset = 0; // Default to 0 if no readings were taken
        Serial.println("Warning: No readings taken for pressureOffset calibration. Defaulting to 0.");
    }
    Serial.print("pressureOffset calibrated: ");
    Serial.println(pressureOffset);
    Serial.println("--------------------------------------------------------------");
}

void DAQ::calibrate(float knownWeight) {
    float rawValue =  getWeightAvg(2500); // Average readings for 2.5 seconds
    if (rawValue != ABERRANT_DATA) {
        calibrationFactor = rawValue / knownWeight;
        Serial.print("Calibration factor set to: ");
        Serial.println(calibrationFactor);
    } else {
        Serial.println("Error: Raw value is mischesque, cannot calibrate.");
    }
}

float DAQ::getWeight() {
    static long digitalOutput = 0;
    static float weight = 0; // in grams
    if (scale.is_ready()) {
        digitalOutput = scale.read() - weightOffset;
        weight = digitalOutput / calibrationFactor; // Convert to weight using the calibration factor
        return weight;
    }
    return ABERRANT_DATA; // Return 99 if scale is not ready or reading is mischesque
}

float DAQ::getPressure() {
    mux.selectChannel(0); // Select the multiplexer channel
  if (isPressureConnected()) { // Check if the sensor is connected
    int16_t rawPressure = readRawPressure(); // Read the digital output
    return pressureToBar(rawPressure); // returns the converted pressure in bar
  }
  else {
    Serial.println("Pressure sensor not connected !");
    delay(1000);
  }
}

float DAQ::getWeightAvg(unsigned long millisToRead) {
    long sum = 0;
    int count = 0;
    unsigned long startTime = millis();

    while (millis() - startTime < millisToRead) {
        if (scale.is_ready()) {
            sum += scale.read() - weightOffset;
            count++;
        }
    }

    return (count > 0) ? (sum / count) : ABERRANT_DATA; // Return average or 99 if no readings
}

void DAQ::fireSequence() {
    Serial.println("Initiating fire sequence...");
    printCountdown(5); // 5 second countdown
    Serial.println("Firing igniter!");
    digitalWrite(IGNITER_PIN, HIGH);
    Serial.println("===============================================================");
    Serial.println("===============================================================");
    // data wrapper printing
    unsigned long startTime = millis();
    unsigned long lastTime = 0;
    while (millis() - startTime < 10000) { // Print data for 10 seconds after firing
        if (millis() - startTime >= 2000) {
            digitalWrite(IGNITER_PIN, LOW); // Turn off igniter after 2 seconds
        }
        if (millis() - lastTime >= 5) { // Print every 5 ms
            lastTime = millis();
            Serial.print(getPressure());
            Serial.print(",");
            Serial.print(getWeight());
            Serial.print(",");
            Serial.print(millis() - startTime);
            Serial.println();
        }

    }
    Serial.println("===============================================================");
    Serial.println("===============================================================");
    Serial.println();
    Serial.println("Fire sequence complete.");
    Serial.println();
    Serial.println("state = RECOVER");
    Serial.println();
    Serial.println("You can close the terminal now !");
    Serial.println();
    Serial.println("===============================================================");
}

void DAQ::resetMux() {
    Serial.println("Resetting PCA9546A...");
    digitalWrite(MUX_RESET_PIN, LOW); // Activate reset (active low)
    delay(10); // Hold reset low for at least 1 ms
    digitalWrite(MUX_RESET_PIN, HIGH); // Release reset
    delay(10);
}

bool DAQ::isPressureConnected() {
    mux.selectChannel(0); // Select channel 0 for pressure sensor
    Wire1.beginTransmission(SENSOR_ADDRESS);
    if (Wire1.endTransmission() == 0) {
        return true; // Sensor acknowledged, it's connected
    }
    return false; // No acknowledgment, sensor not connected
}

int16_t DAQ::readRawPressure() {
    uint16_t pressureData = 0;
    // Request 2 bytes from the pressure register
    Wire1.beginTransmission(SENSOR_ADDRESS);
    Wire1.write(RAM_ADDR_DSP_S); // Send the pressure register address (0x30)
    Wire1.endTransmission(false); // Send a repeated start condition
    // Read 2 bytes of pressure data
    Wire1.requestFrom(SENSOR_ADDRESS, 2);  // Request 2 bytes of data
    if (Wire1.available() >= 2) {
        uint8_t lowByte = Wire1.read(); // Read the low byte
        uint8_t highByte = Wire1.read(); // Read the high byte
        pressureData = (highByte << 8) | lowByte;  // Combine the two bytes
    }
    else {
        return ABERRANT_DATA; // Return 99 if sensor is not responding or data is mischesque
    }
    return (int16_t)pressureData; // Return the digital output
}

float DAQ::pressureToBar(int16_t rawPressure) {
    // Linear interpolation
    if (rawPressure == ABERRANT_DATA) {
        return ABERRANT_DATA; // Return 99 if raw pressure is mischesque
    }
    return ((rawPressure - (-16000.0)) * (100.0) / (16000.0 - (-16000.0)));
}





//================================================================================================================
// Handles serial command input and routes to appropriate gripper control functions
// Parses commands (set, get, cal, arm, disarm, etc.) and updates state machine accordingly
//================================================================================================================
void serial_cmdHandler(DAQ *daq) {

    static const char* cmdStrings[] = {
        "set",
        "get",
        "verbose",
        "loaded",
        "arm",
        "disarm",
        "fire",
        "restart"
    };

    static char cmd[32], arg1[32], arg2[32];

    if (readMessage(cmd, arg1, arg2)) {
        if (strcmp(cmd, cmdStrings[0]) == 0) { // set
            if (strlen(arg1) == 0) {
                Serial.println("Error: Missing setting argument.");
                return;
            }
            if (strcmp(arg1, "test") == 0) { // set test
                if (strlen(arg2) == 0) {
                    Serial.println("Error: Missing test argument.");
                    return;
                }
                Serial.println("argument entered : ");
                Serial.println(arg2);
            }
            if (strcmp(arg1, "weightOffset") == 0) { // set weightOffset
                daq->setWeightOffset();
                Serial.println("command terminated.");
            }
            if (strcmp(arg1, "scale") == 0) { // set scale
                if (strlen(arg2) == 0) {
                    Serial.println("Error: Missing scale value.");
                    return;
                }
                if (currentState != IDLE) {
                    Serial.println("Error: Scale can only be set in IDLE state.");
                    return;
                }
                float scaleValue = atof(arg2);
                if (scaleValue <= 0) {
                    Serial.println("Error: Invalid scale value. Must be a positive number.");
                    return;
                }
                daq->calibrate(scaleValue);
                Serial.println("command terminated.");
            }
            // Handle other commands similarly...
        }
        if (strcmp(cmd, cmdStrings[1]) == 0) { // get
            if (strlen(arg1) == 0) {
                Serial.println("Error: Missing parameter argument.");
                return;
            }
            if (strcmp(arg1, "weight") == 0) { // get weight
                if (currentState != IDLE && currentState != LOADED) {
                    Serial.println("Error: Cannot get weight from current state.");
                    return;
                }
                float weight = daq->getWeightAvg(1000);
                if (weight != ABERRANT_DATA) {
                    Serial.print("Current weight (averaged over 1 second): ");
                    Serial.println(weight);
                } else {
                    Serial.println("Error: Unable to read weight.");
                }
                Serial.println("command terminated.");
            }
            if (strcmp(arg1, "pressure") == 0) { // get pressure
                if (currentState != IDLE && currentState != LOADED) {
                    Serial.println("Error: Cannot get pressure from current state.");
                    return;
                }
                float pressure = daq->getPressure();
                if (pressure != ABERRANT_DATA) {
                    Serial.print("Current pressure: ");
                    Serial.println(pressure);
                } else {
                    Serial.println("Error: Unable to read pressure.");
                }
                Serial.println("command terminated.");
            }
            // Handle other get commands similarly...
        }
        if (strcmp(cmd, cmdStrings[2]) == 0) { // verbose
            if (strlen(arg1) == 0) {
                Serial.println("Error: Missing parameter argument.");
                return;
            }
            if (strcmp(arg1, "loadcell") == 0) { // verbose loadcell
                if (currentState != IDLE && currentState != LOADED) {
                    Serial.println("Error: Cannot enter verbose mode from current state.");
                    return;
                }
                Serial.println("Entering load cell verbose mode. Press any key to exit.");
                currentState = LOAD_CELL_VERBOSE;
            }
            if (strcmp(arg1, "pressure") == 0) { // verbose pressure
                if (currentState != IDLE && currentState != LOADED) {
                    Serial.println("Error: Cannot enter verbose mode from current state.");
                    return;
                }
                Serial.println("Entering pressure verbose mode. Press any key to exit.");
                currentState = PRESSURE_VERBOSE;
            }
            // Handle other verbose commands similarly...
        }
        if (strcmp(cmd, cmdStrings[3]) == 0) { // loaded
            if (currentState != IDLE) {
                Serial.println("Error: Can only load in IDLE state.");
                return;
            }
            currentState = LOADED;
            Serial.println("State changed to LOADED.");
        }
        if (strcmp(cmd, cmdStrings[4]) == 0) { // arm
            if (currentState != LOADED) {
                Serial.println("Error: Can only arm from LOADED state.");
                return;
            }
            currentState = ARMED;
            Serial.println("State changed to ARMED.");
        }
        if (strcmp(cmd, cmdStrings[5]) == 0) { // disarm
            if (currentState != ARMED) {
                Serial.println("Error: Can only disarm from ARMED state.");
                return;
            }
            currentState = LOADED;
            Serial.println("State changed to LOADED.");
        }
        if (strcmp(cmd, cmdStrings[6]) == 0) { // fire
            if (currentState != ARMED) {
                Serial.println("Error: Can only fire from ARMED state.");
                return;
            }
            currentState = FIRE;
            Serial.println("State changed to FIRE.");
        }
        if (strcmp(cmd, cmdStrings[7]) == 0) { // restart
            if (strlen(arg1) == 0) {
                Serial.println("Error: Missing restart argument.");
                return;
            }
            if (strcmp(arg1, "loadcell") == 0) { // restart loadcell
                Serial.println("Restarting load cell...");
                daq->initLoadCell();
                Serial.println("command terminated.");
            }
            if (strcmp(arg1, "pressure") == 0) { // restart pressure
                Serial.println("Restarting pressure sensor...");
                daq->initPressureSensor();
                Serial.println("command terminated.");
            }
        }
    }
    else return;
}

//================================================================================================================
// Reads a line of text from Serial input, handling CR/LF properly
// Parses command format: "<command> <arg1> <arg2>" or "<command> <arg1>" or "<command>"
// Returns true if a complete command was parsed, false if still reading or no data
// Non-blocking: accumulates chars until newline, then parses into outCmd/outArg1/outArg2
//================================================================================================================
bool readMessage(char* outCmd, char* outArg1, char* outArg2) {
    static char buffer[128];
    static int bufferIndex = 0;
    
    // Clear output buffers
    outCmd[0] = '\0';
    outArg1[0] = '\0';
    outArg2[0] = '\0';
    
    // Read available characters
    while (Serial.available() > 0) {
        char c = Serial.read();
        
        // Handle newline (command complete)
        if (c == '\n' || c == '\r') {
            if (bufferIndex > 0) {
                buffer[bufferIndex] = '\0'; // Null-terminate
                
                // Parse the buffer into command and arguments
                char* token = strtok(buffer, " ");
                if (token != nullptr) {
                    strncpy(outCmd, token, 31);
                    outCmd[31] = '\0';
                    
                    token = strtok(nullptr, " ");
                    if (token != nullptr) {
                        strncpy(outArg1, token, 31);
                        outArg1[31] = '\0';
                        
                        token = strtok(nullptr, " ");
                        if (token != nullptr) {
                            strncpy(outArg2, token, 31);
                            outArg2[31] = '\0';
                        }
                    }
                }
                
                bufferIndex = 0; // Reset buffer
                return true; // Command ready
            }
            // Ignore empty lines (just CR/LF)
            continue;
        }
        
        // Accumulate characters
        if (bufferIndex < sizeof(buffer) - 1) {
            buffer[bufferIndex++] = c;
        } else {
            // Buffer overflow - reset
            bufferIndex = 0;
            Serial.print("Error: Command too long");
        }
    }
    
    return false; // No complete command yet
}

void printCountdown(int duration) {
    for (int i = duration; i > 0; i--) {
        Serial.print(i);
        Serial.println("...");
        delay(1000);
    }
}