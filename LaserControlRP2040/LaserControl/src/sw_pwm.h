#ifndef SW_PWM_H
#define SW_PWM_H
#include <Arduino.h>
#include <mbed.h>
#include <chrono>
#include "platform/Callback.h"


using namespace mbed;

class Software_PWM{
    private:
        float frequency;    // in Hz
        float period;       // in s
        float duty_cycle;   // in s
        float timer_period; // in s
        bool running = false;   // indicates if the PWM is running
        std::chrono::microseconds timer_period_us;
        int pin;
        rtos::Thread* thread;
        Timer timer;

        void tick(){
            static bool start = true;
            if(start){
                digitalWrite(this->pin, HIGH);
                start = false;
                timer.reset();
                timer.start();
            }else if (timer.read() >= period){
                digitalWrite(this->pin, LOW);
                start = true;
                timer.stop();
            }

            if(timer.read() >= duty_cycle){
                digitalWrite(this->pin, LOW);
            }
        }

        void loop(){
            while(true){
                if(running){
                    wait_us(timer_period_us.count());
                    tick();
                }else{
                    wait_us(timer_period_us.count()/10);
                    digitalWrite(this->pin, LOW);
                }
            }
        }

        void updateTimerVals(){
            this->timer_period = duty_cycle/10.0;
            this->timer_period_us = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::duration<float>(timer_period));
        }


    public:
        Software_PWM(int pin, rtos::Thread* thread){
            this->pin = pin;
            this->thread = thread;
        }

        void init(){
            pinMode(this->pin, OUTPUT);
            digitalWrite(this->pin, LOW);
            this->timer_period_us = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::duration<float>(100));
            this->thread->start(callback(this, &Software_PWM::loop));
        }

        void start(){
            updateTimerVals(); 
            this->running = true;
        }

        void stop(){
            this->running = false;
            digitalWrite(this->pin, LOW);
        }

        // set frequency in Hz
        void setFrequency(float frequency){
            this->frequency = frequency;
            this->period = 1.0/frequency;
            Serial.print("period: ");
            Serial.println(period, 5);
        }

        // set duty cycle in seconds
        void setDutyCycle(float duty_cycle){
            this->duty_cycle = duty_cycle;
            updateTimerVals();
            Serial.print("duty_cycle: ");
            Serial.println(this->duty_cycle, 5);
            Serial.print("this->timer_period_us: ");
            Serial.println(this->timer_period_us.count());
        }


};

#endif