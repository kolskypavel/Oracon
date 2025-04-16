#include "systemstats.h"

void DeviceStatus::updateBatteryLevel()
{
    // TODO: read from device
    this->battery = 0;
}

long getCurrentTime()
{
    return esp_timer_get_time() / 1000000; // Convert microseconds to seconds
}