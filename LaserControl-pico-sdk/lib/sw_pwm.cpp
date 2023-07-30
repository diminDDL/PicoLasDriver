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
    if(this->period / resolution_divider < 1) {
        this->period = resolution_divider;
    }
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
        sw_pwm->next_positive_width_end = current_time + sw_pwm->positive_width;
        sw_pwm->next_period_end = current_time + sw_pwm->period;
        gpio_put(sw_pwm->pin, 1);
        sw_pwm->pulse_flag = true;
        return true;
    }
    
    if(sw_pwm->pulse_flag && current_time >= sw_pwm->next_positive_width_end) {
        gpio_put(sw_pwm->pin, 0);
        sw_pwm->pulse_flag = false;
        if(sw_pwm->single_shot) {
            sw_pwm->pause_state = true;
            sw_pwm->timer_running = false;
            return false;
        }
    }
    
    if(current_time >= sw_pwm->next_period_end) {
        sw_pwm->last_time = current_time;
        sw_pwm->next_positive_width_end = current_time + sw_pwm->positive_width;
        sw_pwm->next_period_end = current_time + sw_pwm->period;
        gpio_put(sw_pwm->pin, 1);
        sw_pwm->pulse_flag = true;
    }

    return true;
}

void SW_PWM::resume() {
    this->pause_state = false;
    this->last_time = 0;
    this->next_positive_width_end = 0;
    this->next_period_end = 0;

    // if the settings are correct we start the timer
    if(this->enabled && this->period > 0 && this->positive_width > 0 && !this->timer_running) {
        add_repeating_timer_us(-1 * (this->period / resolution_divider), repeating_timer_callback, this, &this->timer);
        this->timer_running = true;
    }
}
