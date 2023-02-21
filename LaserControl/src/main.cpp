#include <Arduino.h>
#include "pwm_lib.h"
#include "tc_lib.h"
#include <DueFlashStorage.h>
#include <comms.h>

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
DueFlashStorage EEPROM;                     // EEPROM object
Communications comms(&Serial, &FORWARD_PORT, SERIAL_BAUD_RATE, SERIAL_MODE); // communications object


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
    Serial.println("Reading EEPROM");
    byte* b = EEPROM.readAddress(4);
    Configuration c;
    memcpy(&c, b, sizeof(Configuration));
    // print the values
    Serial.print("globalpulse: ");
    comms.print_big_int(c.globalpulse);
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
        comms.data.globalPulseCount = c.globalpulse;
        comms.data.setCurrent = c.current;
        comms.data.maxCurrent = c.maxcurr;
        comms.data.setPulseDuration = c.pulsedur;
        comms.data.setPulseFrequency = c.pulsefreq;
    } else {
        Serial.println("CRC incorrect");
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
    uint16_t crc;
    Configuration conf;
    // if 
    if (millis() - lastTime > 1000 || comms.valuesChanged){
        lastTime = millis();
        // read CRC from EEPROM
        uint16_t crcRead = (EEPROM.read(1) << 8) | EEPROM.read(0);
        // set the values to the struct
        conf.globalpulse = comms.data.globalPulseCount;
        conf.current = comms.data.setCurrent;
        conf.maxcurr = comms.data.maxCurrent;
        conf.pulsedur = comms.data.setPulseDuration;
        conf.pulsefreq = comms.data.setPulseFrequency;
        conf.analog = comms.data.analogMode;
        // calculate the CRC
        byte b2[sizeof(Configuration)]; // create byte array to store the struct
        memcpy(b2, &conf, sizeof(Configuration)); // copy the struct to the byte array
        // calculate the crc
        crc = gen_crc16(b2, sizeof(Configuration)); 
        if(crcRead != crc){
            // print the values
            Serial.print("globalpulse: ");
            comms.print_big_int(conf.globalpulse);
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
    comms.readSerial();
    comms.parseBuffer();
    delay(100);
    //EEPROM_service();
    if(!estop){
        // if(valuesChanged){
        //     set_values();
        //     EEPROM_service();
        //     valuesChanged = false;
        // }
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
