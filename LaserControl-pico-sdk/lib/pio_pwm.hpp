#ifndef PIO_PWM_HPP
#define PIO_PWM_HPP

#include <stdio.h>
#include "pico/stdlib.h"
#include "pwm.pio.h"

class PIO_PWM{
    public:
        PIO_PWM(PIO pio, int sm, uint pin);
        void init();
        bool set_all(float freq, uint32_t duty_cycle_us);
        void set_enabled(bool enabled);
        void set_single_shot(bool single_shot);
        void pause();
        void resume();

    private:
        // general use variables
        uint pin;
        float frequency;
        uint32_t duty_cycle_us;
        bool single_shot = false;
        bool enabled = false;
        bool paused = false;
        // raw system stuff for internal use
        PIO pio;
        uint sm;
        uint offset;
        uint16_t clkdiv_int = 2;    // adjust as needed
        uint bitloop_length = 4; // length of bitloop in instructions
        // PIO stuff
        uint pio_level = 0;
        uint pio_period = 0;
        

        void pio_pwm_set_period(PIO pio, uint sm, uint32_t period);
        void pio_pwm_set_level(PIO pio, uint sm, uint32_t level);


};

#endif