#include <Arduino.h>

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("ESP32 CP2102 loopback test");
}

void loop() {
  if(Serial.available()) {
    while(Serial.available()) {
      char c = Serial.read();
      if(c == '\n' || c == '\r') {
        Serial.flush();
        break;
      }else{
        Serial.write(c);
      }
    }
    Serial.write(" loopbacked by esp32-wroom as sportident device\n");
  }
  delay(1);
}