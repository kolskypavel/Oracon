#include <Arduino.h>
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/ssl.h>

extern "C" int wolfSSL_Arduino_Serial_Print(const char *const s) {
    Serial.println(F(s));
    return 0;
}