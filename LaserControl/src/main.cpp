#include <Arduino.h>
#include "pwm_lib.h"
#include "tc_lib.h"
#include <DueFlashStorage.h>

using namespace arduino_due::pwm_lib;
// create a wire object

// TODO
// DAC
// local pulse count
// EEPROM

// configuration variables
// set serial port to 115200 baud, data 8 bits, even parity, 1 stop bit
#define SERIAL_BAUD_RATE 115200
#define SERIAL_MODE SERIAL_8E1
#define FORWARD_PORT Serial1                // when in digital mode all the communication will be forwarded to this port
#define EOL "\r\n"                          // end of line characters
#define INTERRUPT_PIN 2                     // interrupt pin for the trigger
#define ESTOP_PIN 3                         // emergency stop pin, active low
#define EN_PIN 4                            // enable pin, active high
#define PULSE_PIN pwm<pwm_pin::PWML6_PC23>  // pin used to generate the pulses
#define GPIO_BASE_PIN 8                     // base pin for the GPIO outputs
#define GPIO_NUM 4                          // number of GPIO pins
#define OE_LED 13                           // LED indicating wether the output is enabled
#define TRIG_LED 12                         // LED indicating Internal/external triggering
#define E_STOP_LED A3                       // LED indicating that the e-stop is halting the operation of the board
#define FAULT_LED A2                        // LED indicating an internal fault
PULSE_PIN pulsePin;                         // pulse pin object
DueFlashStorage EEPROM;            // EEPROM object

// if estop is true the driver will stop sending pulses to the laser

// the settings we want to store in the EEPROM:
// globalPulseCount
// setCurrent
// maxCurrent
// setPulseDuration
// setPulseFrequency
// analogMode
// the rest are reset on power cycle
struct __attribute__((__packed__)) Configuration {
    uint64_t globalpulse;
    float current;
    float maxcurr;
    uint32_t pulsedur;
    uint32_t pulsefreq;
    bool analog;
};

// variables that need to be remembered between power cycles
uint64_t globalPulseCount = 0;                          // counts the total number of pulses sent do the driver
const String getGlobalPulseCountCommand = "gpsc";       // string representation of the pulse count command
uint64_t localPulseCount = 0;                           // counts the number of pulses sent to the driver since it was turned on
const String getLocalPulseCountCommand = "gcpc";        // string representation of the local pulse count command
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
byte gpioState = 0;                                     // state of the GPIO pins
const String setGpioStateCommand = "stio";              // string representation of the set GPIO state command

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
bool estop = false;             // emergency stop flag
bool valuesChanged = false;     // flag to indicate if the values have changed

unsigned long serialTimeout = 0;
bool newData = false;


void stop(){
}

void trg(){
    // if trigger is HIGH and output is enabled we start PWM
    if (digitalRead(INTERRUPT_PIN) == HIGH && outputEnabled){
        // calculate period in microseconds
        uint32_t period = 1000000 / setPulseFrequency;
        // check if pulse duration is less than period
        if (setPulseDuration < period){
            // stop the old PWM
            pulsePin.stop();
            // start the PWM
            // our numbers are in 1e-6, but the library uses 1e-8
            pulsePin.start(period*100, setPulseDuration*100);
            // set the LED on
            digitalWrite(13, HIGH);
        }
    } else {
        // stop the PWM
        ///PULSE_PIN.stop();
        // set the LED off
        digitalWrite(13, LOW);
    }
}

void pulse(){
    localPulseCount++;
}

void print_big_int(uint64_t value)
{
    if ( value >= 10 )
    {
        print_big_int(value / 10);
    }
    
    Serial.print((uint32_t)(value % 10));
}

#define CRC16 0x8005

uint16_t gen_crc16(const uint8_t *data, uint16_t size)
{
    uint16_t out = 0;
    int bits_read = 0, bit_flag;
    /* Sanity check: */
    if(data == NULL)
        return 0;
    while(size > 0)
    {
        bit_flag = out >> 15;
        /* Get next bit: */
        out <<= 1;
        out |= (*data >> bits_read) & 1; // item a) work from the least significant bits
        /* Increment bit counter: */
        bits_read++;
        if(bits_read > 7)
        {
            bits_read = 0;
            data++;
            size--;
        }
        /* Cycle check: */
        if(bit_flag)
            out ^= CRC16;
    }
    // item b) "push out" the last 16 bits
    int i;
    for (i = 0; i < 16; ++i) {
        bit_flag = out >> 15;
        out <<= 1;
        if(bit_flag)
            out ^= CRC16;
    }
    // item c) reverse the bits
    uint16_t crc = 0;
    i = 0x8000;
    int j = 0x0001;
    for (; i != 0; i >>=1, j <<= 1) {
        if (i & out) crc |= j;
    }
    return crc;
}

void setup() {
    Serial.begin(SERIAL_BAUD_RATE, SERIAL_MODE);
    FORWARD_PORT.begin(SERIAL_BAUD_RATE, SERIAL_MODE);  
    analogWriteResolution(12);
    pinMode(13, OUTPUT);
    for(uint8_t i = 0; i < GPIO_NUM; i++){
        pinMode(GPIO_BASE_PIN + i, OUTPUT);
        digitalWrite(GPIO_BASE_PIN + i, LOW);
    }
    pinMode(ESTOP_PIN, INPUT_PULLUP);   // TODO attach interrupt to this pin
    pinMode(OE_LED, OUTPUT);
    pinMode(TRIG_LED, OUTPUT);
    pinMode(E_STOP_LED, OUTPUT);
    pinMode(FAULT_LED, OUTPUT);

    // set up interrupt to handle the e-stop

    pinMode(INTERRUPT_PIN, INPUT);
    attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN), trg, RISING);
    pinMode(6, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(6), pulse, RISING);
    Serial.println("Reading EEPROM");
    byte* b = EEPROM.readAddress(4);
    Configuration c;
    memcpy(&c, b, sizeof(Configuration));
    // print the values
    Serial.print("globalpulse: ");
    print_big_int(c.globalpulse);
    Serial.println();
    Serial.print("current: ");
    Serial.println(c.current);
    Serial.print("maxcurrent: ");
    Serial.println(c.maxcurr);
    Serial.print("pulseduration: ");
    Serial.println(c.pulsedur);
    Serial.print("pulsefrequency: ");
    Serial.println(c.pulsefreq);
    // read the CRC
    Serial.print("CRC: ");
    uint16_t crcRead = (EEPROM.read(1) << 8) | EEPROM.read(0);
    Serial.println(crcRead, HEX);
    // calculate the CRC
    uint16_t crcCalc = gen_crc16(b, sizeof(Configuration));
    Serial.print("CRC calc: ");
    Serial.println(crcCalc, HEX);
    // check if the CRC is correct
    if (crcRead == crcCalc){
        Serial.println("CRC correct");
        // set the values
        globalPulseCount = c.globalpulse;
        setCurrent = c.current;
        maxCurrent = c.maxcurr;
        setPulseDuration = c.pulsedur;
        setPulseFrequency = c.pulsefreq;
    } else {
        Serial.println("CRC incorrect");
    }
}

void printErrorStr(bool commandType = false){
    // commandType = true - set command
    // commandType = false - get command
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
    if (commandType){
        valuesChanged = true;
    }
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
            if (setCurrent > maxCurrent){
                setCurrent = maxCurrent;
            }
            Serial.print(setCurrent);
            Serial.print(EOL);
            // since printErrorStr is called every time we update the values it updates the values changed flag as well
            printErrorStr(true);
        } else if (strcmp(command, setMaxCurrentCommand.c_str()) == 0){
            // set the max current
            maxCurrent = atof(value);
            Serial.print(maxCurrent);
            Serial.print(EOL);
            printErrorStr(true);
        } else if (strcmp(command, setPulseDurationCommand.c_str()) == 0){
            // set the pulse duration
            setPulseDuration = atol(value);
            Serial.print(setPulseDuration);
            Serial.print(EOL);
            printErrorStr(true);
        } else if (strcmp(command, setPulseFrequencyCommand.c_str()) == 0){
            // set the pulse frequency
            setPulseFrequency = atol(value);
            Serial.print(setPulseFrequency);
            Serial.print(EOL);
            printErrorStr(true);
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
            printErrorStr(true);
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
            printErrorStr(true);
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
            printErrorStr(true);
        } else if(strcmp(command, setGpioStateCommand.c_str()) == 0){ 
            // set the GPIO state 
            gpioState = (atoi(value) > 0) && (atoi(value) < 256) ? atoi(value) : 0;
            Serial.print(gpioState);
            Serial.print(EOL);
            printErrorStr(true);

            Serial.println(gpioState, BIN);
        }else {
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
        } else if (strcmp(str, getLocalPulseCountCommand.c_str()) == 0){
            // get the local pulse count
            print_big_int(localPulseCount);
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
    ///// remove later
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
    Serial.print("GPIO state: ");
    Serial.println(gpioState);
    Serial.print("Changed: ");
    Serial.println(valuesChanged);
    /////
}

void setAnalogCurrentSetpoint(float current){
    // We are using a MCP4725
    Serial.println("Setting analog current");
    // TODO calculate the voltage
    //uint16_t voltage = 0b0000111111111111;
    uint16_t voltage = 0;
}

void set_values(){
    // every time this function is run, the states are pushed to the IO, such as DAC voltages, pulses and so on.
    Serial.println("Setting values");
    // if analog mode is on and output is enabled set the DAC voltage
    if (outputEnabled){
        Serial.println("Output enabled");
        if(analogMode){
            Serial.println("Analog mode");
            // set DAC voltage
            setAnalogCurrentSetpoint(setCurrent);
        }else{
            // Not implemented
        }

        // attach the interrupt pin

    }
    Serial.println("Setting GPIO");
    // GPIOs stay on regardless of OE, only turned off by E-STOP
    for(uint8_t i = 0; i < GPIO_NUM; i++){
        digitalWrite(GPIO_BASE_PIN + i, ((gpioState >> i) & 1));
    }
}

void EEPROM_service(){
    static uint32_t lastTime = 0;
    uint16_t crc;
    Configuration conf;
    // if 
    if (millis() - lastTime > 1000 || valuesChanged){
        lastTime = millis();
        Serial.println("Checking if needs to be saved to EEPROM");

        // read CRC from EEPROM
        Serial.print("CRC EEPROM: ");
        uint16_t crcRead = (EEPROM.read(1) << 8) | EEPROM.read(0);
        Serial.println(crcRead, HEX);
        // set the values to the struct
        conf.globalpulse = globalPulseCount;
        conf.current = setCurrent;
        conf.maxcurr = maxCurrent;
        conf.pulsedur = setPulseDuration;
        conf.pulsefreq = setPulseFrequency;
        conf.analog = analogMode;
        // calculate the CRC
        byte b2[sizeof(Configuration)]; // create byte array to store the struct
        memcpy(b2, &conf, sizeof(Configuration)); // copy the struct to the byte array
        // calculate the crc
        crc = gen_crc16(b2, sizeof(Configuration)); 
        // print the crc
        Serial.print("CRC: ");
        Serial.println(crc, HEX);
        if(crcRead != crc){
            // print the values
            Serial.print("globalpulse: ");
            print_big_int(conf.globalpulse);
            Serial.println();
            Serial.print("current: ");
            Serial.println(conf.current);
            Serial.print("maxcurrent: ");
            Serial.println(conf.maxcurr);
            Serial.print("pulseduration: ");
            Serial.println(conf.pulsedur);
            Serial.print("pulsefrequency: ");
            Serial.println(conf.pulsefreq);
            Serial.print("analog: ");
            Serial.println(conf.analog);

            // write byte array to flash
            EEPROM.write(4, b2, sizeof(Configuration));
            Serial.println("Writing to EEPROM");
            // print out each byte
            for (int i = 0; i < sizeof(Configuration); i++){
                Serial.print(b2[i], HEX);
                Serial.print(" ");
            }
            Serial.println();
            // write the crc to byte 0 and 1
            EEPROM.write(0, crc & 0xFF);
            EEPROM.write(1, (crc >> 8) & 0xFF);
        }
    }
}

void loop() {
    read_host_data(hostBuffer, sizeof(hostBuffer));
    if(newData){
        // Remove later
        Serial.print(hostBuffer);
        //
        newData = false;
        parser(hostBuffer);
    }
    delay(100);
    EEPROM_service();
    if(!estop){
        if(valuesChanged){
            set_values();
            EEPROM_service();
            valuesChanged = false;
        }
    }else{
        // TODO
        // set the DAC to 0V
        // set outputEnabled to false
        // detach the interrupt pin
        // set error flag
        // turn off GPIO
    }
    // if (outputEnabled){
    //     Serial.print("Pulse counter: ");
    //     print_big_int(globalPulseCount);
    //     Serial.println();
    // }
    digitalWrite(OE_LED, outputEnabled);
}


// BOM:
// MCP4725
// 100nF capacitor
// 10uF capacitor
// 1k resistor
// 3.3k resistor
// 2.54mm headers
// 2.54mm raspberry pi header
// screw terminals
// BNC?
// MOLEX connectors (for photodiode)
// TODO parts for photodiode
