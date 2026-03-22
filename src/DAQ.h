#pragma once

#include <arduino.h>
#include "HX711.h"
#include "constants.h"
#include "TCA9548.h"

class DAQ {
    public:
        DAQ();
        void setup();
        void initLoadCell();
        void initPressureSensor();
        void setWeightOffset();
        void setPressureOffset();
        float getWeight();
        float getPressure();
        void calibrate(float knownWeight);
        float getWeightAvg(unsigned long millisToRead);
        void fireSequence();
        void printStatus();
    private:
        // variables
        HX711 *scale = nullptr; // HX711 wrapper object, will be initialized in setup()
        TCA9548 *mux = nullptr; // TCA9548 multiplexer object, will be initialized in setup()
        long weightOffset;
        float pressureOffset;
        float calibrationFactor;
        // functions
        void resetMux();
        bool isPressureConnected();
        int16_t readRawPressure();
        float pressureToBar(int16_t rawPressure);
};

// global variables
extern FSM_STATES currentState;

void update(DAQ *daq);
void serial_cmdHandler(DAQ *daq);
bool readMessage(char* outCmd, char* outArg1, char* outArg2);

void printCountdown(int duration);