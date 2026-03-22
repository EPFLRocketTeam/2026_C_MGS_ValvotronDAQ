
#define CLK_PIN 12
#define DATA_PIN 13

#define MUX_RESET_PIN 33

#define IGNITER_PIN 26

#define ABERRANT_DATA 99 // in case it misch
#define MAX_WEIGHT 1000000 // Maximum expected weight in grams for scaling purposes

// addresses
#define MUX_ADDRESS 0x70  // I2C default for PCA9546A multiplexer
#define SENSOR_ADDRESS 0x6C  // I2C default for PTE7300 sensors
#define RAM_ADDR_DSP_S 0x30  // Pressure register address


enum FSM_STATES {
    IDLE,
    LOADED,
    ARMED,
    FIRE,
    LOAD_CELL_VERBOSE,
    PRESSURE_VERBOSE,
    RECOVER,
    SLEEP
};

