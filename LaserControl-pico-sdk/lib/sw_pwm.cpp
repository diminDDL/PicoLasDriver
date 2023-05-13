#include "sw_pwm.hpp"

#include <stdio.h>

SW_PWM::SW_PWM(uint pin) {
    this->pin = pin;
    this->period = 0;
    this->positive_width = 0;
    this->last_time = 0;
    this->enabled = false;
    this->pause_state = false;
}

void SW_PWM::init() {
    gpio_init(this->pin);
    gpio_set_dir(this->pin, GPIO_OUT);
    gpio_put(this->pin, 0);
}

void SW_PWM::set_duty_cycle_us(uint duty_cycle) {
    this->positive_width = duty_cycle;
}

void SW_PWM::set_freq(uint freq) {
    this->period = 1000000 / freq;
}

void SW_PWM::set_enabled(bool enabled) {
    this->enabled = enabled;
}

void SW_PWM::pause() {
    this->pause_state = true;
    gpio_put(this->pin, 0);
}

bool repeating_timer_callback(struct repeating_timer *t) {
    SW_PWM *sw_pwm = (SW_PWM *)t->user_data;
    if(sw_pwm->pause_state) {
        return true;
    }
    uint64_t current_time = time_us_64();
    if(sw_pwm->last_time == 0) {
        sw_pwm->last_time = current_time;
    }
    if(current_time - sw_pwm->last_time >= sw_pwm->period) {
        sw_pwm->last_time = current_time;
    }
    if(current_time - sw_pwm->last_time < sw_pwm->positive_width) {
        gpio_put(sw_pwm->pin, 1);
    } else {
        gpio_put(sw_pwm->pin, 0);
    }
    return true;
}

void SW_PWM::resume() {
    this->pause_state = false;
    // if the settings are correct we start the timer
    if(this->enabled && this->period > 0 && this->positive_width > 0 && !this->timer_running) {
        add_repeating_timer_us(-1 * (this->period / 10), repeating_timer_callback, this, &this->timer);
        this->timer_running = true;
    }
}

