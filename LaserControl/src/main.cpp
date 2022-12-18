#include <Arduino.h>

// configuration variables
// set serial port to 115200 baud, data 8 bits, even parity, 1 stop bit
#define SERIAL_BAUD_RATE 115200
#define SERIAL_MODE SERIAL_8E1
#define FORWARD_PORT Serial1    // when in digital mode all the communication will be forwarded to this port
#define EOL "\r\n"              // end of line characters
# define DAC_PIN DAC0           // DAC pin

// variables that need to be remembered between power cycles
uint64_t globalPulseCount = 0;                          // counts the total number of pulses sent do the driver
const String getGlobalPulseCountCommand = "gpsc";       // string representation of the pulse count command
float setCurrent = 0.0;                                 // current set by the user in Amperes
const String setCurrentCommand = "scur";                // string representation of the set current command
const String getCurrentCommand = "gcur";                // string representation of the get current command
float maxCurrent = 0.0;                                 // max current set by the user in Amperes
const String setMaxCurrentCommand = "smcu";             // string representation of the set max current command
const String getMaxCurrentCommand = "gmcu";             // string representation of the get max current command
uint32_t setPulseDuration = 0;                          // pulse duration set by the user (in microseconds)
const String setPulseDurationCommand = "spdu";          // string representation of the set pulse duration command
const String getPulseDurationCommand = "gpdu";          // string representation of the get pulse duration command
uint32_t setPulseFrequency = 0;                         // pulse frequency set by the user (in Hz)
const String setPulseFrequencyCommand = "spfr";         // string representation of the set pulse frequency command
const String getPulseFrequencyCommand = "gpfr";         // string representation of the get pulse frequency command
bool outputEnabled = false;                             // output enabled flag
const String enableOutputCommand = "enab";              // string representation of the enable output command
bool lockState = false;                                 // lock state of the driver and UI (locked = true, unlocked = false)
const String lockCommand = "lock";                      // string representation of the lock command
bool analogMode = false;                                // analog mode flag
const String setAnalogModeCommand = "anmo";             // string representation of the analog mode command
const String getModeCommand = "gmod";                   // string representation of the get mode command
// data structure: 
// get commands: <command>\n
// return values: <value>\r\n<00>\r\n
// set commands: <command> <value>\n
// return values: <value>\r\n<00>\r\n
// the \r\n are defined in EOL

// other variables
char incomingByte = 0;      // for incoming serial data
char hostBuffer[32];
char driverBuffer[32];
bool error1 = false;        // Not implemented (should be used to read error states of the driver)
bool error2 = false;
char errorStr[] = "00";

unsigned long serialTimeout = 0;
bool newData = false;

void setup() {
    Serial.begin(SERIAL_BAUD_RATE, SERIAL_MODE);
    FORWARD_PORT.begin(SERIAL_BAUD_RATE, SERIAL_MODE);  
    pinMode(DAC_PIN, OUTPUT);
    analogWriteResolution(12);
}

void printErrorStr(){
    // set the error string
    if (error1){
        errorStr[0] = '1';
    } else {
        errorStr[0] = '0';
    }
    if (error2){
        errorStr[1] = '1';
    } else {
        errorStr[1] = '0';
    }
    // print the error string
    Serial.print(errorStr);
    Serial.print(EOL);
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
    // if the space is in the message that means it's a set command (get commands don't have spaces)
    if (pch != NULL){
        // split the string into command and value by the " " character, also remove all whitespace
        char * command = strtok(str, " ");
        char * value = strtok(NULL, " ");

        // check if the command is known
        if (strcmp(command, setCurrentCommand.c_str()) == 0){
            // set the current
            setCurrent = atof(value);
            Serial.print(setCurrent);
            Serial.print(EOL);
            printErrorStr();
        } else if (strcmp(command, setMaxCurrentCommand.c_str()) == 0){
            // set the max current
            maxCurrent = atof(value);
            Serial.print(maxCurrent);
            Serial.print(EOL);
            printErrorStr();
        } else if (strcmp(command, setPulseDurationCommand.c_str()) == 0){
            // set the pulse duration
            setPulseDuration = atol(value);
            Serial.print(setPulseDuration);
            Serial.print(EOL);
            printErrorStr();
        } else if (strcmp(command, setPulseFrequencyCommand.c_str()) == 0){
            // set the pulse frequency
            setPulseFrequency = atol(value);
            Serial.print(setPulseFrequency);
            Serial.print(EOL);
            printErrorStr();
        } else if (strcmp(command, enableOutputCommand.c_str()) == 0){
            // enable the output, 1 = enable, 0 = disable if the string is invalid it will still return 0 thus disabling the output if the host sends an invalid values
            if (atoi(value) == 1){
                outputEnabled = true;
            } else if (atoi(value) == 0){
                outputEnabled = false;
            } else {
                outputEnabled = false;
            }
            Serial.print(outputEnabled, DEC);
            Serial.print(EOL);
            printErrorStr();
        } else if (strcmp(command, lockCommand.c_str()) == 0){
            // lock the driver, 1 = lock, 0 = unlock if the string is invalid it will still return 0 thus disabling the output if the host sends an invalid values
            if (atoi(value) == 1){
                lockState = true;
            } else if (atoi(value) == 0){
                lockState = false;
            } else {
                lockState = false;
            }
            Serial.print(lockState, DEC);
            Serial.print(EOL);
            printErrorStr();
        } else if (strcmp(command, setAnalogModeCommand.c_str()) == 0){
            // set the driver to analog mode
            if (atoi(value) == 1){
                analogMode = true;
            } else if (atoi(value) == 0){
                analogMode = false;
            } else {
                analogMode = false;
            }
            Serial.print(analogMode, DEC);
            Serial.print(EOL);
            printErrorStr();
        } else {
            // unknown command
            Serial.print("UC");
            Serial.print(EOL);
            printErrorStr();
        }
    }else{
        // if it's a get command perhaps
        // remove all whitespace and new line characters
        char * command = strtok(str, " \r\n");
        // check if the command is known
        if (strcmp(str, getGlobalPulseCountCommand.c_str()) == 0){
            // get the global pulse count
            print_big_int(globalPulseCount);
            Serial.print(EOL);
            printErrorStr();
        } else if (strcmp(str, getCurrentCommand.c_str()) == 0){
            // get the current
            Serial.print(setCurrent);
            Serial.print(EOL);
            printErrorStr();
        } else if (strcmp(str, getMaxCurrentCommand.c_str()) == 0){
            // get the max current
            Serial.print(setCurrent);
            Serial.print(EOL);
            printErrorStr();
        } else if (strcmp(str, getPulseDurationCommand.c_str()) == 0){
            // get the pulse duration
            Serial.print(setPulseDuration);
            Serial.print(EOL);
            printErrorStr();
        } else if (strcmp(str, getPulseFrequencyCommand.c_str()) == 0){
            // get the pulse frequency
            Serial.print(setPulseFrequency);
            Serial.print(EOL);
            printErrorStr();
        } else if (strcmp(str, getModeCommand.c_str()) == 0){
            // get the mode
            Serial.print(analogMode, DEC);
            Serial.print(EOL);
            printErrorStr();
        } else {
            // unknown command
            Serial.print("UC");
            Serial.print(EOL);
            printErrorStr();
        }
    }
    // TODO test cahnges
    Serial.println();
    // print all the new values
    Serial.print("Current: ");
    Serial.println(setCurrent);
    Serial.print("Max current: ");
    Serial.println(maxCurrent);
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

void set_values(){
    // every time this function is run, the states are pushed to the IO, such as DAC voltages, pulses and so on.

    // if analog mode is on and output is enabled set the DAC voltage
    if (outputEnabled){
        if(analogMode){
            // TODO DAC output range is actually no rail to rail so we'll need some circuit to deal with that
            
        }else{
            // Not implemented
        }
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