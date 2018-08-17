/*
 * ####                                                ####
 * ####                                                ####
 * ####                                                ####      ##
 * ####                                                ####    ####
 * ####  ############  ############  ####  ##########  ####  ####
 * ####  ####    ####  ####    ####  ####  ####        ########
 * ####  ####    ####  ####    ####  ####  ####        ########
 * ####  ####    ####  ####    ####  ####  ####        ####  ####
 * ####  ####    ####  ####    ####  ####  ####        ####    ####
 * ####  ############  ############  ####  ##########  ####      ####
 *                             ####                                ####
 * ################################                                  ####
 *            __      __              __              __      __       ####
 *   |  |    |  |    [__)    |_/     (__     |__|    |  |    [__)        ####
 *   |/\|    |__|    |  \    |  \    .__)    |  |    |__|    |             ##
 *
 *
 * DU-INO Arduino Library - Clock Module
 * Aaron Mavrinac <aaron@logick.ca>
 */

#include <TimerOne.h>
#include "du-ino_clock.h"

void clock_isr();

DUINO_Clock::DUINO_Clock()
  : clock_callback_(NULL)
  , external_callback_(NULL)
  , state_(false)
  , retrigger_flag_(false)
  , external_(false)
  , period_(0)
{
}

void DUINO_Clock::begin()
{
  Timer1.initialize();
}

void DUINO_Clock::set_period(unsigned long period)
{
  external_ = false;
  period_ = period;
  update();
}

void DUINO_Clock::set_bpm(uint16_t bpm)
{
  // clock period is microseconds per 16th note
  // bpm is quarter notes per minute (or 16th notes per 15,000,000 us)
  // halved, for on-off cycle
  set_period(7500000 / (unsigned long)bpm);
}

void DUINO_Clock::set_external()
{
  period_ = 0;
  external_ = true;
  update();
}

void DUINO_Clock::on_jack(bool jack_state)
{
  if (!external_)
  {
    set_external();
  }

  on_clock(jack_state);
}

void DUINO_Clock::on_clock(bool jack_state)
{
  state_ = external_ ? jack_state : !state_;

  if (clock_callback_)
  {
    clock_callback_();
  }

  if (state_)
  {
    retrigger_flag_ = true;
  }
}

bool DUINO_Clock::retrigger()
{
  bool flag = retrigger_flag_;
  retrigger_flag_ = false;
  return flag;
}

void DUINO_Clock::update()
{
  Timer1.detachInterrupt();
  state_ = retrigger_flag_ = false;
  if (!external_)
  {
    Timer1.attachInterrupt(clock_isr, period_);
  }
}

DUINO_Clock Clock;

void clock_isr()
{
  Clock.on_clock();
}
