#include <Arduino.h>

// configuration variables
// set serial port to 115200 baud, data 8 bits, even parity, 1 stop bit
#define SERIAL_BAUD_RATE 115200
#define SERIAL_MODE SERIAL_8E1

// variables that need to be remembered between power cycles
uint64_t globalPulseCount = 0;                  // counts the total number of pulses sent do the driver
String getGlobalPulseCountCommand = "gpsc";     // string representation of the pulse count command
float setCurrent = 0.0;                         // current set by the user in Amperes
String setCurrentCommand = "scur";              // string representation of the set current command
String getCurrentCommand = "gcur";              // string representation of the get current command
uint64_t setPulseDuration = 0;                  // pulse duration set by the user (in microseconds)
String setPulseDurationCommand = "spdu";        // string representation of the set pulse duration command
String getPulseDurationCommand = "gpdu";        // string representation of the get pulse duration command
uint32_t setPulseFrequency = 0;                 // pulse frequency set by the user (in Hz)
String setPulseFrequencyCommand = "spfr";       // string representation of the set pulse frequency command
String getPulseFrequencyCommand = "gpfr";       // string representation of the get pulse frequency command
bool outputEnabled = false;                     // output enabled flag
String enableOutputCommand = "enab";            // string representation of the enable output command
String disableOutputCommand = "disa";           // string representation of the disable output command
bool lockState = false;                         // lock state of the driver and UI (locked = true, unlocked = false)
String lockCommand = "lock";                    // string representation of the lock command
String unlockCommand = "unlo";                  // string representation of the unlock command
bool analogMode = false;                        // analog mode flag
String analogModeCommand = "anmo";              // string representation of the analog mode command
String digitalModeCommand = "dimo";             // string representation of the digital mode command
String getModeCommand = "gmod";                 // string representation of the get mode command
// data structure: 
// get commands: <command>\n
// return values: <value>\r\n<00>\r\n
// set commands: <command> <value>\n
// return values: <value>\r\n<00>\r\n

// other variables
char incomingByte = 0;    // for incoming serial data
char hostBuffer[32];
char driverBuffer[32];

unsigned long serialTimeout = 0;
bool newData = false;

void setup() {
    
    Serial.begin(SERIAL_BAUD_RATE, SERIAL_MODE);
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
    // check if there is a " " in the string
    char * pch;
    pch = strchr(str, ' ');
    // if it exists that means it's a set command
    if (pch != NULL){
        // split the string into two parts by the " " character
        char * command = strtok(str, " ");
        char * value = strtok(NULL, " ");
        // print the command and value
        Serial.print("Command: ");
        Serial.println(command);
        Serial.print("Value: ");
        Serial.println(value);
    }

}

void loop() {
    read_host_data(hostBuffer, sizeof(hostBuffer));
    if(newData){
        Serial.print(hostBuffer);
        newData = false;
        parser(hostBuffer);
    }
    delay(100);
}