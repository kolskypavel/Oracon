#include "systemstats.h"

void DeviceStatus::updateBatteryLevel()
{ 
    // TODO: fix with voltage divider
    float voltage = analogRead(BATTERY_MEASURE_PORT);
    
    this->battery = 0;
}

long getCurrentTime()
{
    return esp_timer_get_time() / 1000000; // Convert microseconds to seconds
}