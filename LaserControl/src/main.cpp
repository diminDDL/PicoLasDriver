#include <Arduino.h>

void setup() {
    Serial.begin(115200);
}

void loop() {
    // serial echo
    if (Serial.available()) {
        Serial.write(Serial.read());
        delay(100);
    }
    // Serial.println("Hello World!");
    // delay(500);
}