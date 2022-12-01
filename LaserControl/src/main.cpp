#include <Arduino.h>

// configuration variables
// set serial port to 115200 baud, data 8 bits, even parity, 1 stop bit
#define SERIAL_BAUD_RATE 115200
#define SERIAL_MODE SERIAL_8E1
#define FORWARD_PORT Serial1    // when in digital mode all the communication will be forwarded to this port

// variables that need to be remembered between power cycles
uint64_t globalPulseCount = 0;                  // counts the total number of pulses sent do the driver
const String getGlobalPulseCountCommand = "gpsc";     // string representation of the pulse count command
float setCurrent = 0.0;                         // current set by the user in Amperes
const String setCurrentCommand = "scur";              // string representation of the set current command
const String getCurrentCommand = "gcur";              // string representation of the get current command
uint64_t setPulseDuration = 0;                  // pulse duration set by the user (in microseconds)
const String setPulseDurationCommand = "spdu";        // string representation of the set pulse duration command
const String getPulseDurationCommand = "gpdu";        // string representation of the get pulse duration command
uint32_t setPulseFrequency = 0;                 // pulse frequency set by the user (in Hz)
const String setPulseFrequencyCommand = "spfr";       // string representation of the set pulse frequency command
const String getPulseFrequencyCommand = "gpfr";       // string representation of the get pulse frequency command
bool outputEnabled = false;                     // output enabled flag
const String enableOutputCommand = "enab";            // string representation of the enable output command
const String disableOutputCommand = "disa";           // string representation of the disable output command
bool lockState = false;                         // lock state of the driver and UI (locked = true, unlocked = false)
const String lockCommand = "lock";                    // string representation of the lock command
const String unlockCommand = "unlo";                  // string representation of the unlock command
bool analogMode = false;                        // analog mode flag
const String analogModeCommand = "anmo";              // string representation of the analog mode command
const String digitalModeCommand = "dimo";             // string representation of the digital mode command
const String getModeCommand = "gmod";                 // string representation of the get mode command
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
    FORWARD_PORT.begin(SERIAL_BAUD_RATE, SERIAL_MODE);
}

void print_big_int(uint64_t value)
{
    if ( value >= 10 )
    {
        print_big_int(value / 10);
    }
    
    Serial.print((uint32_t)(value % 10));
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
    // if we are in digital mode forward the data to the driver and record the response
    if (!analogMode){
        // forward the data to the driver
        FORWARD_PORT.print(str);
        // wait a bit for the response
        delay(50);
        // read the response from the driver
        uint8_t i = 0;
        while (FORWARD_PORT.available() > 0) {
            driverBuffer[i] = (char)FORWARD_PORT.read();
            i++;
            if (i >= 32) {
                break;
            }
        }
    }
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
        // check if the command is known
        if (strcmp(command, setCurrentCommand.c_str()) == 0){
            // set the current
            setCurrent = atof(value);
            Serial.print("Current set to: ");
            Serial.println(setCurrent);
        } else if (strcmp(command, setPulseDurationCommand.c_str()) == 0){
            // set the pulse duration
            setPulseDuration = atol(value);
            Serial.print("Pulse duration set to: ");
            print_big_int(setPulseDuration);
        } else if (strcmp(command, setPulseFrequencyCommand.c_str()) == 0){
            // set the pulse frequency
            setPulseFrequency = atol(value);
            Serial.print("Pulse frequency set to: ");
            Serial.println(setPulseFrequency);
        } else if (strcmp(command, enableOutputCommand.c_str()) == 0){
            // enable the output
            outputEnabled = true;
            Serial.println("Output enabled");
        } else if (strcmp(command, disableOutputCommand.c_str()) == 0){
            // disable the output
            outputEnabled = false;
            Serial.println("Output disabled");
        } else if (strcmp(command, lockCommand.c_str()) == 0){
            // lock the driver
            lockState = true;
            Serial.println("Driver locked");
        } else if (strcmp(command, unlockCommand.c_str()) == 0){
            // unlock the driver
            lockState = false;
            Serial.println("Driver unlocked");
        } else if (strcmp(command, analogModeCommand.c_str()) == 0){
            // set the driver to analog mode
            analogMode = true;
            Serial.println("Analog mode enabled");
        } else if (strcmp(command, digitalModeCommand.c_str()) == 0){
            // set the driver to digital mode
            analogMode = false;
            Serial.println("Digital mode enabled");
        } else {
            // unknown command
            Serial.println("Unknown command");
        }
        
        // print all the new values
        Serial.print("Current: ");
        Serial.println(setCurrent);
        Serial.print("Pulse duration: ");
        print_big_int(setPulseDuration);
        Serial.println();
        Serial.print("Pulse frequency: ");
        Serial.println(setPulseFrequency);
        Serial.print("Output enabled: ");
        Serial.println(outputEnabled);
        Serial.print("Lock state: ");
        Serial.println(lockState);
        Serial.print("Analog mode: ");
        Serial.println(analogMode);

    }

}

void set_values(){}

void loop() {
    read_host_data(hostBuffer, sizeof(hostBuffer));
    if(newData){
        Serial.print(hostBuffer);
        newData = false;
        parser(hostBuffer);
    }
    delay(100);
}