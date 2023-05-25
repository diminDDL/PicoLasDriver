#include <stdio.h>
#include "pico/stdio.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/vreg.h"
#include "blink.pio.h"
#include "pico/rand.h"
#include "pico/multicore.h"
#include "hardware/pwm.h"
#include "lib/utils.h"
#include "lib/pwm.hpp"
#include "lib/sw_pwm.hpp"
#include "hardware/spi.h"
#include "lib/fram.hpp"
#include "lib/comms.hpp"

#include "definitions.h"

// create new PWM instance
PWM hw_PWM(PULSE_PIN, pwm_gpio_to_slice_num(PULSE_PIN));
SW_PWM sw_PWM(TRIG_LED);
FRAM fram(FRAM_SPI, FRAM_SPI_CS);
// TODO 
// test fram
// test sw_pwm
// port serial parser
// write mcp DAC driver

int main() {
    set_sys_clock_khz(100000, true);
    stdio_init_all();

    gpio_init(25);
    gpio_set_dir(25, GPIO_OUT);
    gpio_put(25, 1);

    static const uint led_pin = 9;
    static const float pio_freq = 10000;

    // PIO pio = pio0;
    // uint sm = pio_claim_unused_sm(pio, true);
    // uint offset = pio_add_program(pio, &blink_program);
    // float div = (float)clock_get_hz(clk_sys) / pio_freq;
    // blink_program_init(pio, sm, offset, led_pin, div);
    // pio_sm_set_enabled(pio, sm, true);

    // generate a 100 Hz square wave on pin TRIG_LED
    // gpio_set_function(TRIG_LED, GPIO_FUNC_PWM);
    // uint slice_num = pwm_gpio_to_slice_num(TRIG_LED);
    // pwm_set_wrap(slice_num, 62500);
    // pwm_set_chan_level(slice_num, pwm_gpio_to_channel(TRIG_LED), 31250);
    // pwm_set_clkdiv(slice_num, (float)1600.0);
    // pwm_set_enabled(slice_num, true);

    gpio_set_function(FRAM_SPI_SCLK, GPIO_FUNC_SPI);
    gpio_set_function(FRAM_SPI_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(FRAM_SPI_MISO, GPIO_FUNC_SPI);
    gpio_init(FRAM_SPI_CS);
    gpio_set_dir(FRAM_SPI_CS, GPIO_OUT);
    gpio_put(FRAM_SPI_CS, true);
    fram.init();

    hw_PWM.init();
    sleep_ms(2000);
    sw_PWM.init();
    // Do nothing
    while (true) {
        printf("100Hz 50%\n");
        hw_PWM.set_duty_cycle(50);
        hw_PWM.set_freq(1);
        hw_PWM.set_enabled(true);
        printf("sw 100Hz 50%\n");
        sw_PWM.set_duty_cycle_us(500);
        printf("sw part2\n");
        sw_PWM.set_freq(100);
        printf("sw part3\n");
        sw_PWM.set_enabled(true);
        printf("sw part4\n");
        sw_PWM.resume();

        sleep_ms(10000);
        printf("stopped\n");
        hw_PWM.pause();
        sw_PWM.pause();
        sleep_ms(10000);
        printf("resumed\n");
        hw_PWM.resume();
        sw_PWM.resume();
        sleep_ms(10000);


        // print all internals of hw_PWM
        printf("system clock: %d\n", clock_get_hz(clk_sys));
        printf("pin: %d\n", hw_PWM.pin);
        printf("slice_num: %d\n", hw_PWM.slice_num);
        printf("freq: %d\n", hw_PWM.freq);
        printf("duty_cycle: %d\n", hw_PWM.duty_cycle);
        printf("duty_cycle_us: %d\n", hw_PWM.duty_cycle_us);
        printf("div: %f\n", hw_PWM.div);
        printf("enabled: %d\n", hw_PWM.enabled);
        printf("level: %d\n", hw_PWM.level);
        printf("wrap: %d\n", hw_PWM.wrap);
        printf("resolution_us: %d\n", hw_PWM.resolution_us);
        printf("pause_state: %d\n", hw_PWM.pause_state);

        fram.setBP(true, true);
        fram.setWEL(true);
        printf("%d\n", fram.getStatus());
        fram.setWEL(false);
        fram.setBP(false, true);
        printf("%d\n", fram.getStatus());
        fram.setWEL(false);
        fram.setBP(false, false);
        printf("%d\n", fram.getStatus());
        //fram.write(0x155, 0x69);
        printf("%d\n", fram.read(0x155));
        //fram.write(0x166, 0x96);
        printf("%d\n", fram.read(0x166));

        const char data[] = "Hello World!";
        char data2[13];

        fram.write((uint16_t)0x0, (uint8_t *)data, (uint16_t)sizeof(data));
        fram.read((uint16_t)0x0, (uint8_t *)data2, (uint16_t)sizeof(data2));

        printf("%s\n", data2);        

        // sleep_ms(10000);
        // printf("100Hz 25%\n");
        // hw_PWM.set_duty_cycle(25);
        // sleep_ms(10000);
        // printf("100Hz 75%\n");
        // hw_PWM.set_duty_cycle(75);
        // sleep_ms(10000);
        // printf("1000Hz 75%\n");
        // hw_PWM.set_freq(1000);
        // sleep_ms(10000);
        // printf("stopped\n");
        // hw_PWM.set_enabled(false);
        // sleep_ms(10000);
        // printf("resumed\n");
        // hw_PWM.set_enabled(true);
        // sleep_ms(10000);
        if(get_bootsel_button()){
            reset_usb_boot(0, 0);
        }
    }
}