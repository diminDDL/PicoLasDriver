#include <Arduino.h>

// variables that need to be remembered between power cycles
uint64_t globalPulseCount = 0;
float setCurrent = 0.0;
uint32_t setPulseDuration = 0;
uint32_t setPulseFrequency = 0;

// other variables
char incomingByte = 0;    // for incoming serial data
char hostBuffer[32];
char driverBuffer[32];

unsigned long serialTimeout = 0;
bool newData = false;

void setup() {
    Serial.begin(115200);    // opens serial port

}

// Reads the Serial port and stores it into dest.
void read_host_data(char dest[], uint8_t size) {
    // read the data from serial port and store it in the dest array
    uint8_t i = 0;
    if (Serial.available() >0){
        // clear the buffer
        memset(dest, 0, size);
    }
    while (Serial.available() > 0) {
        dest[i] = (char)Serial.read();
        i++;
        newData = true;
        if (i >= size) {
            break;
        }
    }
}

// parses the input string and sets the variables
void parser(char str[]){

}

void loop() {
    read_host_data(hostBuffer, sizeof(hostBuffer));
    if(newData){
        Serial.print(hostBuffer);
        newData = false;
    }
    delay(100);
}