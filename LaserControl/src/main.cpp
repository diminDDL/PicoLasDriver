#include <Arduino.h>
#include "pwm_lib.h"
#include "tc_lib.h"
#include "Wire.h"
#include "MCP4725.h"
#include <comms.h>
#include <memory.h>

using namespace arduino_due::pwm_lib;
// create a wire object

// TODO
// local pulse count
// reading ADC
// estop
// add R32 short
// external/no trigger

// configuration variables
// set serial port to 115200 baud, data 8 bits, even parity, 1 stop bit
#define SERIAL_BAUD_RATE 115200
#define SERIAL_MODE SERIAL_8E1
#define FORWARD_PORT Serial1                // when in digital mode all the communication will be forwarded to this port
#define INTERRUPT_PIN 2                     // interrupt pin for the trigger
#define ESTOP_PIN 3                         // emergency stop pin, active low
#define EN_PIN 4                            // enable pin, active high
#define PULSE_PIN pwm<pwm_pin::PWML6_PC23>  // pin used to generate the pulses
#define PULSE_PIN_STD 7                     // pin used to generate the pulses in standard notation
#define PULSE_COUNT_PIN 6                   // pin used to count the pulses
#define GPIO_BASE_PIN 8                     // base pin for the GPIO outputs
#define GPIO_NUM 4                          // number of GPIO pins
#define OE_LED 13                           // LED indicating wether the output is enabled
#define TRIG_LED 12                         // LED indicating Internal/external triggering
#define E_STOP_LED A3                       // LED indicating that the e-stop is halting the operation of the board
#define FAULT_LED A2                        // LED indicating an internal fault
#define MAX_DAC_VOLTAGE 1.5                 // maximum voltage of the DAC
#define DAC_VREF 3.3                        // DAC reference voltage
#define DAC_VDIV 2                          // DAC voltage divider ratio 2 means we are using a 1:2 divider aka the output is half the input
PULSE_PIN pulsePin;                         // pulse pin object
Communications comms(&Serial, &FORWARD_PORT, SERIAL_BAUD_RATE, SERIAL_MODE); // communications object
Memory memory;                              // memory object
MCP4725 MCP(0x60, &Wire1);                  // init DAC         

// if estop is true the driver will stop sending pulses to the laser

bool estop = false;             // emergency stop flag

void stop(){
}
uint32_t pwm_period;
void trg(){
    // if trigger is HIGH and output is enabled we start PWM
    if (digitalRead(INTERRUPT_PIN) == HIGH){
        pulsePin.start();
        digitalWrite(TRIG_LED, HIGH);
    } else {
        // stop the PWM
        pulsePin.stop();
        digitalWrite(TRIG_LED, LOW);
    }
}

void pulse(){
    comms.data.globalPulseCount++;
}

void setup() {
    //Serial.begin(SERIAL_BAUD_RATE, SERIAL_MODE);
    //FORWARD_PORT.begin(SERIAL_BAUD_RATE, SERIAL_MODE);  
    comms.init();
    Serial.println("Starting");
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
    pinMode(PULSE_PIN_STD, OUTPUT);
    digitalWrite(PULSE_PIN_STD, LOW);

    // set up interrupt to handle the e-stop

    // set up the PWM
    pinMode(INTERRUPT_PIN, INPUT);

    pinMode(PULSE_COUNT_PIN, INPUT);
    attachInterrupt(digitalPinToInterrupt(PULSE_COUNT_PIN), pulse, RISING);

    if(memory.loadCurrent() == true){
        // read values from memory
        comms.data.globalPulseCount = memory.config.globalpulse;
        comms.data.setCurrent = memory.config.current;
        comms.data.maxCurrent = memory.config.maxcurr;
        comms.data.setPulseDuration = memory.config.pulsedur;
        comms.data.setPulseFrequency = memory.config.pulsefreq;
        comms.data.analogMode = memory.config.analog;
    }else{
        Serial.println("Could not load memory");
    }

    if (MCP.begin() == false)
    {
      Serial.println("Could not attach to DAC");
    }
}

void setAnalogCurrentSetpoint(float current){
    // We are using a MCP4725
    // TODO maybe add calibration constants
    uint16_t raw = 0;
    float volts = (current * MAX_DAC_VOLTAGE / comms.data.maxCurrent) * DAC_VDIV;
    raw = (uint16_t)(volts * 4095 / DAC_VREF);
    MCP.setValue(raw);
}

void set_values(){
    // every time this function is run, the states are pushed to the IO, such as DAC voltages, pulses and so on.
    // if analog mode is on and output is enabled set the DAC voltage
    if (comms.data.outputEnabled){
        if(comms.data.analogMode){
            // set DAC voltage
            setAnalogCurrentSetpoint(comms.data.setCurrent);
            delay(10); // allow the DAC to settle
        }else{
            // Not implemented
        }

        // calculate period in microseconds
        pwm_period = 1000000 / (comms.data.setPulseFrequency);
        // attach the interrupt pin
        // check if pulse duration is less than period
        if (comms.data.setPulseDuration < pwm_period){
            // our numbers are in 1e-6, but the library uses 1e-8
            pulsePin.stop();
            pulsePin.start(pwm_period*100, (comms.data.setPulseDuration)*100, false);
            pulsePin.stop();
            attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN), trg, CHANGE);
        }
    }else{
        pulsePin.stop();
        digitalWrite(PULSE_PIN_STD, LOW);
        detachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN));
        
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
        // TODO rewrite this to use the memory object
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
        // send error message
    }
    // if (comms.data.outputEnabled){
    //     Serial.print("Pulse counter: ");
    //     comms.print_big_int(comms.data.globalPulseCount);
    //     Serial.println();
    // }
    digitalWrite(OE_LED, comms.data.outputEnabled);
}
