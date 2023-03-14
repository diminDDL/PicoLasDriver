#include <Arduino.h>
#include "pwm_lib.h"
#include "tc_lib.h"
#include <comms.h>
#include <memory.h>

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
Communications comms(&Serial, &FORWARD_PORT, SERIAL_BAUD_RATE, SERIAL_MODE); // communications object
Memory memory;                              // memory object


// if estop is true the driver will stop sending pulses to the laser

bool estop = false;             // emergency stop flag

void stop(){
}

void trg(){
    // if trigger is HIGH and output is enabled we start PWM
    if (digitalRead(INTERRUPT_PIN) == HIGH && comms.data.outputEnabled){
        // calculate period in microseconds
        uint32_t period = 1000000 / comms.data.setPulseFrequency;
        // check if pulse duration is less than period
        if (comms.data.setPulseDuration < period){
            // stop the old PWM
            pulsePin.stop();
            // start the PWM
            // our numbers are in 1e-6, but the library uses 1e-8
            pulsePin.start(period*100, comms.data.setPulseDuration*100);
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
    comms.data.localPulseCount++;
}

void setup() {
    //Serial.begin(SERIAL_BAUD_RATE, SERIAL_MODE);
    //FORWARD_PORT.begin(SERIAL_BAUD_RATE, SERIAL_MODE);  
    comms.init();
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
    // Serial.println("Reading EEPROM");
    // byte* b = EEPROM.readAddress(4);
    // Configuration c;
    // memcpy(&c, b, sizeof(Configuration));
    // // print the values
    // Serial.print("globalpulse: ");
    // comms.print_big_int(c.globalpulse);
    // Serial.println();
    // Serial.print("current: ");
    // Serial.println(c.current);
    // Serial.print("maxcurrent: ");
    // Serial.println(c.maxcurr);
    // Serial.print("pulseduration: ");
    // Serial.println(c.pulsedur);
    // Serial.print("pulsefrequency: ");
    // Serial.println(c.pulsefreq);
    // // read the CRC
    // Serial.print("CRC: ");
    // uint16_t crcRead = (EEPROM.read(1) << 8) | EEPROM.read(0);
    // Serial.println(crcRead, HEX);
    // // calculate the CRC
    // uint16_t crcCalc = gen_crc16(b, sizeof(Configuration));
    // Serial.print("CRC calc: ");
    // Serial.println(crcCalc, HEX);
    // // check if the CRC is correct
    // if (crcRead == crcCalc){
    //     Serial.println("CRC correct");
    //     // set the values
    //     comms.data.globalPulseCount = c.globalpulse;
    //     comms.data.setCurrent = c.current;
    //     comms.data.maxCurrent = c.maxcurr;
    //     comms.data.setPulseDuration = c.pulsedur;
    //     comms.data.setPulseFrequency = c.pulsefreq;
    // } else {
    //     Serial.println("CRC incorrect");
    // }
    bool success = memory.readPage(0);
    if(success){
        // read values from memory
        comms.data.globalPulseCount = memory.config.globalpulse;
        comms.data.setCurrent = memory.config.current;
        comms.data.maxCurrent = memory.config.maxcurr;
        comms.data.setPulseDuration = memory.config.pulsedur;
        comms.data.setPulseFrequency = memory.config.pulsefreq;
        comms.data.analogMode = memory.config.analog;
    }else{
        // write empty config
        //memory.writePage(0);
    }
    Serial.println("Starting");
    delay(5000);
    Serial.println("Complete");

    for(uint8_t i = 0; i < 50; i++){
        Serial.print("Writing PAGES ");
        Serial.println(i);
        memory.config.current = i;
        memory.writeLeveled();
        memory.loadCurrent();
        Serial.print("Reading PAGES ");
        Serial.println(i);
        Serial.print("Current: ");
        Serial.println(memory.config.current);
        // if(i == 32){
        //     Serial.println("=====================");
        //     Serial.println("MEMORY DUMP");
        //     for (uint8_t i = 0; i < 10; i++){
        //         Serial.print("Page ");
        //         Serial.print(i);
        //         Serial.print(": ");
        //         Serial.println(memory.readPage(i, true));
        //     }
        //     Serial.println("Dump complete");
        //     Serial.println("=====================");
        // }
        Serial.println("--------------------");
    }
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
    if (comms.data.outputEnabled){
        Serial.println("Output enabled");
        if(comms.data.analogMode){
            Serial.println("Analog mode");
            // set DAC voltage
            setAnalogCurrentSetpoint(comms.data.setCurrent);
        }else{
            // Not implemented
        }

        // attach the interrupt pin

    }
    Serial.println("Setting GPIO");
    // GPIOs stay on regardless of OE, only turned off by E-STOP
    for(uint8_t i = 0; i < GPIO_NUM; i++){
        digitalWrite(GPIO_BASE_PIN + i, ((comms.data.gpioState >> i) & 1));
    }
}

void EEPROM_service(){
    static uint32_t lastTime = 0;
    if (millis() - lastTime > 1000 || comms.valuesChanged){
        lastTime = millis();
        // set the values
        memory.config.globalpulse = comms.data.globalPulseCount;
        memory.config.current = comms.data.setCurrent;
        memory.config.maxcurr = comms.data.maxCurrent;
        memory.config.pulsedur = comms.data.setPulseDuration;
        memory.config.pulsefreq = comms.data.setPulseFrequency;
        memory.config.analog = comms.data.analogMode;
        memory.writePage(0);
    }
}

void loop() {
    comms.readSerial();
    comms.parseBuffer();
    delay(100);
    //EEPROM_service();
    if(!estop){
        if(comms.valuesChanged){
            set_values();
            //EEPROM_service();
            comms.valuesChanged = false;
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
    digitalWrite(OE_LED, comms.data.outputEnabled);
}
