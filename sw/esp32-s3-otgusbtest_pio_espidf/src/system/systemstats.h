#include <stdint.h>

struct Status{
    uint8_t battery;
    uint8_t signal;
    uint16_t punchesReceived;
};

int getBatteryStatus();
