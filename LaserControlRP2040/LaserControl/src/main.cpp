#include <Arduino.h>
#include "Wire.h"
#include <comms.h>
//TODO figure out EEPROM
//#include <memory.h>
// create a wire object

// TODO
// lock state?
// test it
// fix bug where frequency < trigger freq causes the driver to lag and behave weirdly

// configuration variables
// set serial port to 115200 baud, data 8 bits, even parity, 1 stop bit
#define SERIAL_BAUD_RATE 115200
#define SERIAL_MODE SERIAL_8E1
#define FORWARD_PORT Serial1                // when in digital mode all the communication will be forwarded to this port
#define INTERRUPT_PIN 2                     // interrupt pin for the trigger
#define ESTOP_PIN 3                         // emergency stop pin, active low
#define EN_PIN 4                            // enable pin, active high
#define PULSE_PIN 5  // pin used to generate the pulses
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
#define ADC_PIN A0                          // pin used to read the photodiode

Communications comms = Communications();

// if estop is true the driver will stop sending pulses to the laser

bool estop = false;             // emergency stop flag
bool eeprom_fault = false;      // eeprom fault flag

void stop();

uint32_t pwm_period;
// why is this firing very often on low frequencies
void trg(){
    detachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN));
    digitalWrite(TRIG_LED, HIGH);
    static bool running = false;

    // if trigger is HIGH and output is enabled we start PWM
    if (digitalRead(INTERRUPT_PIN) == HIGH && running == false){
        // TODO pulsePin.start();
        running = true;
        //digitalWrite(TRIG_LED, HIGH);
    } else {
        // stop the PWM
        // TODO pulsePin.stop();
        running = false;
        //digitalWrite(TRIG_LED, LOW);
    }
    digitalWrite(57, running);
    digitalWrite(TRIG_LED, LOW);
    attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN), trg, CHANGE);
}

void pulse(){
    if(digitalRead(PULSE_COUNT_PIN) == HIGH){
        comms.data.globalPulseCount++;
    }
}

void dummy_pulse(){
    // we do nothing here
    __asm__ __volatile__ ("nop");
}

void setup() {
    //Serial.begin(SERIAL_BAUD_RATE, SERIAL_MODE);
    //FORWARD_PORT.begin(SERIAL_BAUD_RATE, SERIAL_MODE);  
    comms.init();
    Serial.println("Starting");
    analogWriteResolution(12);
    analogReadResolution(12);
    for(uint8_t i = 0; i < GPIO_NUM; i++){
        pinMode(GPIO_BASE_PIN + i, OUTPUT);
        digitalWrite(GPIO_BASE_PIN + i, LOW);
    }
    pinMode(EN_PIN, OUTPUT);
    digitalWrite(EN_PIN, HIGH);
    pinMode(ESTOP_PIN, INPUT);   // TODO attach interrupt to this pin
    attachInterrupt(digitalPinToInterrupt(ESTOP_PIN), dummy_pulse, RISING);
    detachInterrupt(digitalPinToInterrupt(ESTOP_PIN));
    delay(1);
    attachInterrupt(digitalPinToInterrupt(ESTOP_PIN), stop, FALLING);
    pinMode(OE_LED, OUTPUT);
    pinMode(TRIG_LED, OUTPUT);
    pinMode(E_STOP_LED, OUTPUT);
    pinMode(FAULT_LED, OUTPUT);
    pinMode(PULSE_PIN_STD, OUTPUT);
    pinMode(57, OUTPUT);
    digitalWrite(PULSE_PIN_STD, LOW);

    // set up interrupt to handle the e-stop

    // set up the PWM
    pinMode(INTERRUPT_PIN, INPUT);

    pinMode(PULSE_COUNT_PIN, INPUT);
    attachInterrupt(digitalPinToInterrupt(PULSE_COUNT_PIN), dummy_pulse, RISING);
    detachInterrupt(digitalPinToInterrupt(PULSE_COUNT_PIN));

    // TODO if(memory.loadCurrent() == true){
    //     // read values from memory
    //     comms.data.globalPulseCount = memory.config.globalpulse;
    //     comms.data.initPulseCount = memory.config.globalpulse;
    //     comms.data.setCurrent = memory.config.current;
    //     comms.data.maxCurrent = memory.config.maxcurr;
    //     comms.data.setPulseDuration = memory.config.pulsedur;
    //     comms.data.setPulseFrequency = memory.config.pulsefreq;
    //     comms.data.analogMode = memory.config.analog;
    // }else{
    //     Serial.println("Could not load memory");
    //     eeprom_fault = true;
    // }

    // TODO if (MCP.begin() == false)
    // {
    //   Serial.println("Could not attach to DAC");
    // }
}

void setAnalogCurrentSetpoint(float current){
    // We are using a MCP4725
    // TODO maybe add calibration constants
    uint16_t raw = 0;
    float volts = (current * MAX_DAC_VOLTAGE / comms.data.maxCurrent) * DAC_VDIV;
    raw = (uint16_t)(volts * 4095 / DAC_VREF);
    // TODO MCP.setValue(raw);
}

void set_values(){
    static bool enabled = false;
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
        if (comms.data.setPulseDuration <= pwm_period){
            // our numbers are in 1e-6, but the library uses 1e-8
            Serial.println("Starting PWM");
            // TODO pulsePin.stop();
            // pulsePin.start(pwm_period*100, (comms.data.setPulseDuration)*100, false);
            // pulsePin.stop();
            if(!enabled){
                // this cursed code is to prevent the interrupt from firing when we attach it
                attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN), trg, CHANGE);
                delay(1);
                attachInterrupt(digitalPinToInterrupt(PULSE_COUNT_PIN), pulse, CHANGE);
                enabled = true;
            }
            // edge case, if the trigger is already high we need to start the PWM
            if(digitalRead(INTERRUPT_PIN) == HIGH){
                trg();
            }
        }
    }else{
        // TODO pulsePin.stop();
        digitalWrite(PULSE_PIN_STD, LOW);
        digitalWrite(TRIG_LED, LOW);
        detachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN));
        detachInterrupt(digitalPinToInterrupt(PULSE_COUNT_PIN));
        enabled = false;
    }
    Serial.println("Setting GPIO");
    // GPIOs stay on regardless of OE, only turned off by E-STOP
    for(uint8_t i = 0; i < GPIO_NUM; i++){
        digitalWrite(GPIO_BASE_PIN + i, ((comms.data.gpioState >> i) & 1));
    }
}

void EEPROM_service(){
    static uint32_t lastTime = 0;
    if (millis() - lastTime > 10000 || comms.valuesChanged){
        lastTime = millis();
        // set the values
        // TODO memory.config.globalpulse = comms.data.globalPulseCount;
        // memory.config.current = comms.data.setCurrent;
        // memory.config.maxcurr = comms.data.maxCurrent;
        // memory.config.pulsedur = comms.data.setPulseDuration;
        // memory.config.pulsefreq = comms.data.setPulseFrequency;
        // memory.config.analog = comms.data.analogMode;

        // write to memory
        // TODO eeprom_fault = !memory.writeLeveled();
    }
}

void pollADC(){
    comms.data.adcValue = analogRead(ADC_PIN);
}

void stop(){
    // turn off the output
    digitalWrite(EN_PIN, LOW);
    // set the DAC to 0V
    // TODO MCP.setValue(0);
    // set outputEnabled to false
    comms.data.outputEnabled = false;
    // detach the interrupt pin
    detachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN));
    detachInterrupt(digitalPinToInterrupt(PULSE_COUNT_PIN));
    // stop the PWM
    // TODO pulsePin.stop();
    // set error flag
    comms.error2 = true;
    // turn off GPIO
    for(uint8_t i = 0; i < GPIO_NUM; i++){
        digitalWrite(GPIO_BASE_PIN + i, LOW);
    }
    // send error message
    // TODO implement error message
    // comms.sendError();
    // turn on LED
    digitalWrite(FAULT_LED, HIGH);
    // force store memory
    if(!estop){
        comms.valuesChanged = true;
        EEPROM_service();
        comms.valuesChanged = false;
        Serial.println("Stopping");
    }
    estop = true;
}

void loop() {
    pollADC();
    comms.readSerial();
    comms.parseBuffer();
    delay(100);
    EEPROM_service();
    if(!estop){
        if(comms.valuesChanged){
            Serial.println("Values changed");
            set_values();
            comms.valuesChanged = false;
        }
    }else{
        stop();
    }
    // if (comms.data.outputEnabled){
    //     Serial.print("Global Pulse counter: ");
    //     comms.print_big_int(comms.data.globalPulseCount);
    //     Serial.print("; Local: ");
    //     comms.print_big_int(comms.data.localPulseCount);
    //     Serial.println();
    // }
    digitalWrite(OE_LED, comms.data.outputEnabled);
    digitalWrite(E_STOP_LED, estop);
    if(eeprom_fault || comms.error2){
        digitalWrite(FAULT_LED, HIGH);
    }else{
        digitalWrite(FAULT_LED, LOW);
    }
}
