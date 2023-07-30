#include "pio_pwm.hpp"
#include "hardware/clocks.h"
#include "hardware/sync.h"
#include "hardware/watchdog.h"
#include <stdio.h>

uint *global_level_ptr;
bool *single_shot_ptr;
bool *paused_ptr;
PIO *global_pio_ptr;
uint *global_sm_ptr;

void pwm_pio_irq(){
    uint32_t status = save_and_disable_interrupts();
    watchdog_update();
    // dereference the pointers to get the values
    uint level = *global_level_ptr;
    bool single_shot = *single_shot_ptr;
    bool paused = *paused_ptr;
    PIO pio = *global_pio_ptr;
    uint sm = *global_sm_ptr;
    // if we are in single shot we don't put anything into the FIFO to stall the PIO
    if(!single_shot && !paused){
        pio_sm_put(pio, sm, level);
        //pio_sm_put(pio, sm, level);
    }
    pio0_hw->irq = 1;           // TODO deal with this later
    restore_interrupts(status);
}

// TODO this only works with PIO0 SM0 due to how the interrupts are setup
PIO_PWM::PIO_PWM(PIO pio, int sm, uint pin){
    this->pio = pio;
    this->sm = sm;
    this->pin = pin;
    // shove the pointers into the global variables
    global_level_ptr = &this->pio_level;
    single_shot_ptr = &this->single_shot;
    paused_ptr = &this->paused;
    global_pio_ptr = &this->pio;
    global_sm_ptr = &this->sm;
}

void PIO_PWM::init(){
    this->offset = pio_add_program(pio, &pwm_program);
    pwm_program_init(this->pio, this->sm, this->offset, this->pin);
    pio_sm_set_clkdiv_int_frac(this->pio, this->sm, this->clkdiv_int, 0);
    irq_set_exclusive_handler(PIO0_IRQ_0, pwm_pio_irq);
    irq_set_enabled(PIO0_IRQ_0, true);
    pio0_hw->inte0 = PIO_IRQ0_INTE_SM0_BITS;
}

void PIO_PWM::set_enabled(bool enabled){
    this->enabled = enabled;
    pio_sm_set_enabled(this->pio, this->sm, this->enabled);
}

void PIO_PWM::pause(){
    this->paused = true;
    gpio_init(this->pin);
    gpio_set_dir(this->pin, GPIO_OUT);
}

void PIO_PWM::resume(){
    this->paused = false;
    pio_pwm_set_level(this->pio, this->sm, this->pio_level);
    gpio_set_function(this->pin, GPIO_FUNC_PIO0);
}

void PIO_PWM::set_single_shot(bool single_shot){
    this->single_shot = single_shot;
}

void PIO_PWM::pio_pwm_set_period(PIO pio, uint sm, uint32_t period) {
    pio_sm_set_enabled(pio, sm, false);
    pio_sm_put_blocking(pio, sm, period);
    pio_sm_exec(pio, sm, pio_encode_pull(false, false));
    pio_sm_exec(pio, sm, pio_encode_out(pio_isr, 32));
    pio_sm_set_enabled(pio, sm, this->enabled);
}

void PIO_PWM::pio_pwm_set_level(PIO pio, uint sm, uint32_t level) {
    pio_sm_put_blocking(pio, sm, level);
}

bool PIO_PWM::set_all(float freq, uint32_t duty_cycle_us){

    this->frequency = freq;                 // frequency in Hz
    this->duty_cycle_us = duty_cycle_us;    // duty cycle in microseconds
    this->pio_period = 0;                   // Period as a count

    uint32_t pwm_core_freq = clock_get_hz(clk_sys) / this->bitloop_length / clkdiv_int; // PWM frequency
    const uint32_t max_uint32 = 0xFFFFFFFF; // Maximum 32-bit unsigned integer
    uint divisor = 1;

    while (1) {
        float scaled_freq = frequency * divisor;
        float period_sec = 1 / scaled_freq;
        this->pio_period = (uint32_t)(period_sec * pwm_core_freq);                          // Period as a count
        this->pio_level = (this->pio_period) - ((uint32_t)(duty_cycle_us * (pwm_core_freq / (1e6 * divisor))));    // Level as a count

        if (this->pio_period <= max_uint32 && this->pio_level <= max_uint32) {
            this->pio_pwm_set_period(this->pio, this->sm, this->pio_period);
            this->pio_pwm_set_level(this->pio, this->sm, this->pio_level);
            return true;
        }

        divisor *= 2;
        if (divisor > max_uint32) {
            // Failed to find suitable divisor. The requested frequency and/or duty cycle might be too low.
            return false;
        }
    }
    return false;
}
