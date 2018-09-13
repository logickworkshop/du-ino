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
  , swung_(false)
  , external_(false)
  , count_(-1)
  , swung_ms_(0)
  , period_(0)
  , swing_(0)
  , divider_(1)
  , div_count_(0)
{
}

void DUINO_Clock::begin()
{
  Timer1.initialize();
  OCR0A = 0xAF;
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

void DUINO_Clock::set_swing(uint8_t swing)
{
  if (swing > 6)
  {
    swing = 6;
  }
  swing_ = swing;

  if (swing_)
  {
    TIMSK0 |= _BV(OCIE0A);
  }
  else
  {
    TIMSK0 &= ~(_BV(OCIE0A));
  }
}

void DUINO_Clock::set_divider(uint8_t divider)
{
  if(divider > 0 && divider < 65)
  {
    divider_ = divider;
  }
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
    
    if (external_callback_)
    {
      external_callback_();
    }
  }

  if (state_ != jack_state)
  {
    on_clock();
  }
}

void DUINO_Clock::on_clock()
{
  if (!external_ && swing_ && !state_ && count_ % 2)
  {
    // wait an extra (period_ / 1000) * (4 * swing_ / 25) milliseconds on 2 & 4
    swung_ms_ = millis() + ((period_ * swing_) / 6250);
    swung_ = true;
  }
  else
  {
    toggle_state();
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

void DUINO_Clock::reset()
{
  swung_ = false;
  count_ = -1;
  div_count_ = 0;
  update();
  on_clock();
}

void DUINO_Clock::check_swing()
{
  if (swung_ && millis() > swung_ms_)
  {
    swung_ = false;
    toggle_state();
  }
}

void DUINO_Clock::toggle_state()
{
  // divide clock
  div_count_++;
  div_count_ %= 64;
  if (div_count_ < divider_)
  {
    return;
  }
  div_count_ = 0;

  state_ = !state_;

  if (state_)
  {
    count_++;
    count_ %= 64;
    retrigger_flag_ = true;
  }

  if (clock_callback_)
  {
    clock_callback_();
  }
}

DUINO_Clock Clock;

void clock_isr()
{
  Clock.on_clock();
}

ISR(TIMER0_COMPA_vect)
{
  Clock.check_swing();
}
