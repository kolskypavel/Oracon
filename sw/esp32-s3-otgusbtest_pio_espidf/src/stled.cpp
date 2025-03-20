#include "stled.h"

#include <Arduino.h>
#include <FastLED.h>

#include "esp_log.h"

const static char *TAG = "stled";

CRGB leds[1];

void stled_setup(){
    FastLED.addLeds<WS2812B, 48, GRB>(leds, 1);
    FastLED.setBrightness(25);
}

void stled_loop(){
    leds[0] = CRGB(0, 255, 255);
    FastLED.show();
    //ESP_LOGI(TAG, "indicate mode reset");
}