#ifndef SW_PWM_H
#define SW_PWM_H

#include <stdio.h>
#include "pico/stdlib.h"

class SW_PWM {
public:
    SW_PWM(uint pin);
    void init();
    void set_duty_cycle_us(uint duty_cycle);
    void set_freq(uint freq);
    void set_enabled(bool enabled);
    void pause();
    void resume();
    bool pause_state = false;
    uint64_t last_time = 0;
    uint pin = 0;
    uint64_t period = 0;
    uint64_t positive_width = 0;
    bool timer_running = false;
    bool single_shot = false;
    bool enabled = false;
    bool pulse_flag = false;

    private:
    struct repeating_timer timer;
    static const uint32_t resolution_divider = 100;
    
};

#endif // SW_PWM_H
