#include <stdio.h>
#include "pico/stdio.h"
#include "pico/rand.h"
#include "pico/multicore.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/vreg.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "hardware/adc.h"
#include "hardware/spi.h"
#include "blink.pio.h"
#include "lib/utils.h"
#include "lib/pwm.hpp"
#include "lib/sw_pwm.hpp"
#include "lib/comms.hpp"
#include "lib/memory.hpp"
#include "lib/mcp4725/mcp4725.hpp"
#include "definitions.h"

// TODO
// pulse mode - single or continuous
// handle errors
// EEPROM weirdness with the new pulse type thing
// random estops

// create new PWM instance
SW_PWM sw_PWM(PULSE_PIN);
Communications comms = Communications();
Memory memory = Memory(FRAM_SPI, FRAM_SPI_CS);
MCP4725_PICO MCP(DAC_VREF);

bool estop = false;             // emergency stop flag
bool eeprom_fault = false;      // eeprom fault flag
uint32_t pwm_period;

void set_values();
void trg();
void pulse();
void setAnalogCurrentSetpoint(float current);
void set_values();
void EEPROM_service();
void pollADC();
void gpio_callback(uint gpio, uint32_t events);
void stop();

int main() {
    vreg_set_voltage(VREG_VOLTAGE_MAX);
    set_sys_clock_khz(400000, true);
    stdio_init_all();

    gpio_init(25);
    gpio_set_dir(25, GPIO_OUT);
    gpio_put(25, 1);

    adc_init();
    adc_gpio_init(ADC_PIN_PD);

    for(uint8_t i = 0; i < GPIO_NUM; i++){
        gpio_init(GPIO_BASE_PIN + i);
        gpio_set_dir(GPIO_BASE_PIN + i, GPIO_OUT);
        gpio_put(GPIO_BASE_PIN + i, 0);
    }
    gpio_init(EN_PIN);
    gpio_set_dir(EN_PIN, GPIO_IN);
    gpio_set_pulls(EN_PIN, true, false);
    gpio_init(ESTOP_PIN);
    gpio_set_dir(ESTOP_PIN, GPIO_IN);
    gpio_set_pulls(ESTOP_PIN, true, false);
    sleep_ms(1);
    gpio_set_irq_enabled_with_callback(ESTOP_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);
    gpio_init(OE_LED);
    gpio_set_dir(OE_LED, GPIO_OUT);
    gpio_init(TRIG_LED);
    gpio_set_dir(TRIG_LED, GPIO_OUT);
    gpio_init(E_STOP_LED);
    gpio_set_dir(E_STOP_LED, GPIO_OUT);
    gpio_init(FAULT_LED);
    gpio_set_dir(FAULT_LED, GPIO_OUT);

    sw_PWM.init();
    sw_PWM.set_enabled(true);

    gpio_init(INTERRUPT_PIN);
    gpio_set_dir(INTERRUPT_PIN, GPIO_IN);
    gpio_init(PULSE_COUNT_PIN);
    gpio_set_dir(PULSE_COUNT_PIN, GPIO_IN);

    gpio_set_function(FRAM_SPI_SCLK, GPIO_FUNC_SPI);
    gpio_set_function(FRAM_SPI_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(FRAM_SPI_MISO, GPIO_FUNC_SPI);
    gpio_init(FRAM_SPI_CS);
    gpio_set_dir(FRAM_SPI_CS, GPIO_OUT);
    gpio_put(FRAM_SPI_CS, true);
    memory.init(); 
    
    comms.init();

    // test all LEDs
    for(uint8_t i = 0; i < 2; i++){
        gpio_put(OE_LED, 1);
        gpio_put(E_STOP_LED, 1);
        gpio_put(FAULT_LED, 1);
        gpio_put(TRIG_LED, 1);
        gpio_put(25, 1);
        sleep_ms(100);
        gpio_put(OE_LED, 0);
        gpio_put(E_STOP_LED, 0);
        gpio_put(FAULT_LED, 0);
        gpio_put(TRIG_LED, 0);
        gpio_put(25, 0);
        sleep_ms(100);
    }
    gpio_put(25, 1);
    sleep_ms(3000);

    if(memory.loadCurrent() == true){
        // read values from memory
        //printf("Reading memory\n");
        comms.data.globalPulseCount = memory.config.globalpulse;
        //printf("Global pulse count:");
        //comms.print_big_int(comms.data.globalPulseCount);
        comms.data.initPulseCount = memory.config.globalpulse;
        //printf("\nInit pulse count:");
        //comms.print_big_int(comms.data.initPulseCount);
        comms.data.setCurrent = memory.config.current;
        //printf("\nCurrent: %f\n", comms.data.setCurrent);
        comms.data.maxCurrent = memory.config.maxcurr;
        //printf("Max current: %f\n", comms.data.maxCurrent);
        comms.data.setPulseDuration = memory.config.pulsedur;
        //printf("Pulse duration: %d\n", comms.data.setPulseDuration);
        comms.data.setPulseFrequency = memory.config.pulsefreq;
        //printf("Pulse frequency: %d\n", comms.data.setPulseFrequency);
        comms.data.analogMode = memory.config.analog;
        //printf("Analog mode: %d\n", comms.data.analogMode);
        comms.data.pulseMode = memory.config.pulseMode; 
        //printf("Pulse mode: %d\n", comms.data.pulseMode);
    }else{
        printf("EEPROM fault\n");
        eeprom_fault = true;
    }

    if (!MCP.begin(MCP4725A0_Addr_A00, i2c0, 100, DAC_SDA, DAC_SCL)){
        printf("Could not attach to DAC\n");
    }
    // set the DAC to 0V
    MCP.setInputCode(0, MCP4725_FastMode, MCP4725_PowerDown_Off);

    comms.valuesChanged = true;     // force the values to be sent to everything

    bool stopped = false;

    while (true) {

        pollADC();
        comms.readSerial();
        comms.parseBuffer();
        EEPROM_service();
        if(!estop){
            if(comms.valuesChanged){
                set_values();
                comms.valuesChanged = false;
            }
        }else{
            if(!stopped){
                stop();
                comms.estopFlag = true;
                stopped = true;
            }
        }
        // if (comms.data.outputEnabled){
        //     Serial.print("Global Pulse counter: ");
        //     comms.print_big_int(comms.data.globalPulseCount);
        //     Serial.print("; Local: ");
        //     comms.print_big_int(comms.data.localPulseCount);
        //     Serial.println();
        // }
        gpio_put(OE_LED, comms.data.outputEnabled);
        gpio_put(E_STOP_LED, estop);

        if(eeprom_fault || comms.error2){
            gpio_put(FAULT_LED, 1);
        }else{
            gpio_put(FAULT_LED, 0);
        }
        sleep_ms(100);

        if(get_bootsel_button()){
            reset_usb_boot(0, 0);
        }
    }
}


void pollADC(){
    adc_select_input(ADC_MUX_PD);
    comms.data.adcValue = adc_read();
}

void setAnalogCurrentSetpoint(float current){
    //printf("Setting DAC to %f\n", current);
    uint16_t raw = 0;
    float volts = (current * MAX_DAC_VOLTAGE / comms.data.maxCurrent) * DAC_VDIV;
    raw = (uint16_t)(volts * 4095 / DAC_VREF);
    MCP.setInputCode(raw, MCP4725_FastMode, MCP4725_PowerDown_Off);
    // print raw value
    //printf("Raw value: %d\n", raw);
}

// TODO why it only writes to 1 and 0?
void EEPROM_service(){
    static uint64_t lastTime = 0;
    
    if(comms.eraseFlag){
        comms.eraseFlag = false;
        printf("Erasing memory\n");
        memory.erase();
    }

    if (time_us_64() - lastTime > 10000000 || comms.valuesChanged){
        lastTime = time_us_64();
        // printf("Writing memory\n");
        // write to memory struct
        memory.config.globalpulse = comms.data.globalPulseCount;
        memory.config.current = comms.data.setCurrent;
        memory.config.maxcurr = comms.data.maxCurrent;
        memory.config.pulsedur = comms.data.setPulseDuration;
        memory.config.pulsefreq = comms.data.setPulseFrequency;
        memory.config.analog = comms.data.analogMode;
        memory.config.pulseMode = comms.data.pulseMode;
        // commit to memory
        eeprom_fault = !memory.writeLeveled();
    }
}

void set_values(){
    static bool enabled = false;
    // every time this function is run, the states are pushed to the IO, such as DAC voltages, pulses and so on.
    // if analog mode is on and output is enabled set the DAC voltage
    if (comms.data.outputEnabled){
        if(comms.data.analogMode){
            // set DAC voltage
            setAnalogCurrentSetpoint(comms.data.setCurrent);
            sleep_ms(10); // allow the DAC to settle
        }else{
            // digital mode not implemented
        }
        // calculate period in microseconds
        pwm_period = (1000000 / (comms.data.setPulseFrequency));
        // attach the interrupt pin
        // check if pulse duration is less than period
        if (comms.data.setPulseDuration < pwm_period){
            // our numbers are in 1e-6, but the library uses 1e-8
            // printf("Starting PWM\n");
            // printf("Period: ");
            // printf("%d\n", pwm_period);
            // printf("Pulse Duration: ");
            // printf("%d\n", comms.data.setPulseDuration);
            // printf("Pulse Frequency: ");
            // printf("%d\n", comms.data.setPulseFrequency);

            sw_PWM.set_freq(comms.data.setPulseFrequency);
            sw_PWM.set_duty_cycle_us(comms.data.setPulseDuration);
            if(comms.data.pulseMode == 1){
                sw_PWM.single_shot = true;
            }else{
                sw_PWM.single_shot = false;
            }
            sw_PWM.pause();
        }else{
            // printf("Pulse duration is longer than period");
        }

        if(!enabled){
            gpio_set_irq_enabled(INTERRUPT_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
            gpio_set_irq_enabled(PULSE_COUNT_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
            enabled = true;
        }

        gpio_set_dir(EN_PIN, GPIO_OUT);
        gpio_put(EN_PIN, 1);

    }else{
        sw_PWM.pause();
        gpio_put(PULSE_PIN, 0);
        gpio_put(TRIG_LED, 0);
        gpio_set_dir(EN_PIN, GPIO_IN);
        gpio_set_pulls(EN_PIN, true, false);
        gpio_set_irq_enabled(INTERRUPT_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, false);
        gpio_set_irq_enabled(PULSE_COUNT_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, false);
        enabled = false;
    }

    for(uint8_t i = 0; i < GPIO_NUM; i++){
        gpio_put(GPIO_BASE_PIN + i, ((comms.data.gpioState >> i) & 1));
    }
}

void stop(){
    // printf("Stopping\n");
    // turn off the output
    gpio_set_dir(EN_PIN, GPIO_OUT);
    gpio_put(EN_PIN, 0);
    // set the DAC to 0V
    MCP.setInputCode(0, MCP4725_FastMode, MCP4725_PowerDown_Off);
    // set outputEnabled to false
    comms.data.outputEnabled = false;
    // detach the interrupt pin
    gpio_set_irq_enabled(INTERRUPT_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, false);
    gpio_set_irq_enabled(PULSE_COUNT_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, false);
    // stop the PWM
    sw_PWM.pause();
    sw_PWM.set_enabled(false);
    // set error flag
    comms.error2 = true;
    // turn off GPIO
    for(uint8_t i = 0; i < GPIO_NUM; i++){
        gpio_put(GPIO_BASE_PIN + i, 0);
    }
    // send error message
    // TODO implement error message
    // comms.sendError();
    // turn on LED
    gpio_put(FAULT_LED, 1);
    // force store memory
    if(!estop){
        comms.valuesChanged = true;
        EEPROM_service();
        comms.valuesChanged = false;
        printf("%s%s", comms.estop, EOL);
    }
    estop = true;
}

void trg(){
    static bool running = false;
    // if trigger is HIGH and output is enabled we start PWM
    if (gpio_get(INTERRUPT_PIN) == 1 && running == false){
        gpio_put(TRIG_LED, 1);
        sw_PWM.resume();
        running = true;
    } else {
        // stop the PWM
        sw_PWM.pause();
        gpio_put(TRIG_LED, 0);
        gpio_put(PULSE_PIN, 0);
        running = false;
    }
}

void pulse(){
    if(gpio_get(PULSE_COUNT_PIN) == 1){
        comms.data.globalPulseCount++;
    }
}

void gpio_callback(uint gpio, uint32_t events){
    if(gpio == ESTOP_PIN){
        stop();
    } else if(gpio == INTERRUPT_PIN){
        trg();
    } else if(gpio == PULSE_COUNT_PIN){
        pulse();
    }
}
