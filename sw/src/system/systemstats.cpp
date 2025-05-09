#include "systemstats.h"

void DeviceStatus::updateBatteryLevel()
{
    // 12bit ADC - 4096 steps; 3.3V reference voltage, 3.3V/4096 = 0.0008056640625V per step
    // Multiply by 1000 to get mV, mutliply by 2 to get voltage from a 1:1 voltage divider

    // 4.2V full charge
    // 3.7V ~50%
    // 3.3V 0% - because of voltage regulator, really discharged is around 2.5 V

    // Todo: add compensation / set ADC params for better approx
    float voltage = analogRead(BATTERY_MEASURE_PORT);
    this->battery = voltage * (MIN_BATTERY_LEVEL / BATTERY_ADC_STEPS) * 1000 * 2;
}

long getCurrentTime()
{
    return esp_timer_get_time() / 1000000; // Convert microseconds to seconds
}