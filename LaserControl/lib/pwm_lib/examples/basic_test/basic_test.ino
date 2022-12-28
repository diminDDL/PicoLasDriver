/**
 ** pwm_lib library
 ** Copyright (C) 2015,2020
 **
 **   Antonio C. Domínguez Brito <antonio.dominguez@ulpgc.es>
 **     División de Robótica y Oceanografía Computacional <www.roc.siani.es>
 **     and Departamento de Informática y Sistemas <www.dis.ulpgc.es>
 **     Universidad de Las Palmas de Gran  Canaria (ULPGC) <www.ulpgc.es>
 **  
 ** This file is part of the pwm_lib library.
 ** The pwm_lib library is free software: you can redistribute it and/or modify
 ** it under  the  terms of  the GNU  General  Public  License  as  published  by
 ** the  Free Software Foundation, either  version  3  of  the  License,  or  any
 ** later version.
 ** 
 ** The  pwm_lib library is distributed in the hope that  it  will  be  useful,
 ** but   WITHOUT   ANY WARRANTY;   without   even   the  implied   warranty   of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR  PURPOSE.  See  the  GNU  General
 ** Public License for more details.
 ** 
 ** You should have received a copy  (COPYING file) of  the  GNU  General  Public
 ** License along with the pwm_lib library.
 ** If not, see: <http://www.gnu.org/licenses/>.
 **/
/*
 * File: basic_test.ino 
 * Description: This is a basic example illustrating the use of li-
 * brary pwm_lib. It generates two PWM signals with different periods
 * and duties (pulse durations).
 * Date: December 20th, 2015
 * Author: Antonio C. Dominguez-Brito <antonio.dominguez@ulpgc.es>
 * ROC-SIANI - Universidad de Las Palmas de Gran Canaria - Spain
 */

#include "pwm_lib.h"
#include "tc_lib.h"

using namespace arduino_due::pwm_lib;

#define PWM_PERIOD_PIN_35 800000000 // hundredth of usecs (1e-8 secs)
#define PWM_DUTY_PIN_35 50000000 // 100 msecs in hundredth of usecs (1e-8 secs)

#define PWM_PERIOD_PIN_42 10000 // 100 usecs in hundredth of usecs (1e-8 secs)
#define PWM_DUTY_PIN_42 1000 // 10 usec in hundredth of usecs (1e-8 secs)

#define CAPTURE_TIME_WINDOW 15000000 // usecs
#define DUTY_KEEPING_TIME 30000 // msecs 

// defining pwm object using pin 35, pin PC3 mapped to pin 35 on the DUE
// this object uses PWM channel 0
pwm<pwm_pin::PWMH0_PC3> pwm_pin35;

// defining pwm objetc using pin 42, pin PA19 mapped to pin 42 on the DUE
// this object used PWM channel 1
pwm<pwm_pin::PWMH1_PA19> pwm_pin42;

// To measure PWM signals generated by the previous pwm objects, we will use
// capture objects of tc_lib library as "oscilloscopes" probes. We will need
// a different capture object for measuring each signal: capture_tc0 and
// capture_tc1, respectively.
// IMPORTANT: Take into account that for TC0 (TC0 and channel 0) the TIOA0 is
// PB25, which is pin 2 for Arduino DUE, so capture_tc0's capture pin is pin
// 2. For capture_tc1 (TC0 and channel 1), TIOA1 is PA2 which is pin A7
// (ANALOG IN 7). For the correspondence between all TIOA inputs for the 
// different TC module channels, you should consult uC Atmel ATSAM3X8E 
// datasheet in section "36. Timer Counter (TC)"), and the Arduino pin mapping 
// for the DUE.
// All in all, to meausure pwm outputs in this example you should connect the 
// PWM output of pin 35 to capture_tc0 object pin 2, and pwm output pin 42 to 
// capture_tc1's pin A7.

capture_tc0_declaration(); // TC0 and channel 0
auto& capture_pin2=capture_tc0;

capture_tc1_declaration(); // TC0 and channel 1
auto& capture_pinA7=capture_tc1;

void setup() {
  // put your setup code here, to run once:

  Serial.begin(9600);

  // initialization of capture objects
  capture_pin2.config(CAPTURE_TIME_WINDOW);
  capture_pinA7.config(CAPTURE_TIME_WINDOW);

  // starting PWM signals
  pwm_pin35.start(PWM_PERIOD_PIN_35,PWM_DUTY_PIN_35);
  pwm_pin42.start(PWM_PERIOD_PIN_42,PWM_DUTY_PIN_42);

  Serial.println("================================================");
  Serial.println("============= pwm_lib - basic_test =============");
  Serial.println("================================================");
  for(size_t i=0; i<=pwm_core::max_clocks;i++) 
  {
    Serial.print("maximum period - clock "); Serial.print(i); Serial.print(": ");
    Serial.print(pwm_core::max_period(i),12); Serial.println(" s."); 
  }
  Serial.println("================================================");
}

// FIX: function template change_duty is defined in
// #define to avoid it to be considered a function
// prototype when integrating all .ino files in one
// whole .cpp file. Without this trick the compiler
// complains about the definition of the template
// function.
#define change_duty_definition \
template<typename pwm_type> void change_duty( \
  pwm_type& pwm_obj, \
  uint32_t pwm_duty, \
  uint32_t pwm_period \
) \
{ \
  uint32_t duty=pwm_obj.get_duty()+pwm_duty; \
  if(duty>pwm_period) duty=pwm_duty; \
  pwm_obj.set_duty(duty); \
}
// FIX: here we instantiate the template definition
// of change_duty
change_duty_definition;

void loop() {
  // put your main code here,to run repeatedly:

  delay(DUTY_KEEPING_TIME);

  uint32_t status,duty,period;

  Serial.println("================================================");
  status=capture_pin2.get_duty_and_period(duty,period);
  Serial.print("[PIN 35 -> PIN 2] duty: "); 
  Serial.print(
    static_cast<double>(duty)/
    static_cast<double>(capture_pin2.ticks_per_usec()),
    3
  );
  Serial.print(" usecs. period: ");
  Serial.print(period/capture_pin2.ticks_per_usec());
  Serial.println(" usecs.");

  status=capture_pinA7.get_duty_and_period(duty,period);
  Serial.print("[PIN 42 -> PIN A7] duty: "); 
  Serial.print(
    static_cast<double>(duty)/
    static_cast<double>(capture_pinA7.ticks_per_usec()),
    3
  );
  Serial.print(" usecs. period: ");
  Serial.print(period/capture_pinA7.ticks_per_usec());
  Serial.println(" usecs.");
  Serial.println("================================================");

  // changing duty in pwm output pin 35 
  change_duty(pwm_pin35,PWM_DUTY_PIN_35,PWM_PERIOD_PIN_35);

  // changing duty in pwm output pin 42 
  change_duty(pwm_pin42,PWM_DUTY_PIN_42,PWM_PERIOD_PIN_42);
}

