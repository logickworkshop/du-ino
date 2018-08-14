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
 * DU-INO Arduino Library - Click Encoder Module
 * Aaron Mavrinac <aaron@logick.ca>
 */

#include <avr/interrupt.h>
#include "du-ino_encoder.h"

// encoder acceleration configuration for 1000Hz tick
#define ENC_ACCEL_TOP                                   3072
#define ENC_ACCEL_INC                                     25
#define ENC_ACCEL_DEC                                      2

// button configuration for 1000Hz tick
#define ENC_BUTTONINTERVAL                                10
#define ENC_DOUBLECLICKTIME                              600
#define ENC_HOLDTIME                                    1200

DUINO_Encoder::DUINO_Encoder(uint8_t a, uint8_t b, uint8_t btn)
  : pin_a_(a)
  , pin_b_(b)
  , pin_btn_(btn)
  , delta_(0)
  , last_(0)
  , acceleration_(0)
  , button_(Open)
{
  // configure pins for active-low operation
  pinMode(pin_a_, INPUT_PULLUP);
  pinMode(pin_b_, INPUT_PULLUP);
  pinMode(pin_btn_, INPUT_PULLUP);

  // initialize state
  if (digitalRead(pin_a_) == LOW)
  {
    last_ = 3;
  }
  if (digitalRead(pin_b_) == LOW)
  {
    last_ ^= 1;
  }

  // set up timer
  TIMSK2 &= ~(1 << TOIE2);
  TCCR2A &= ~((1 << WGM21) | (1 << WGM20));
  TCCR2B &= ~(1 << WGM22);
  ASSR &= ~(1 << AS2);
  TIMSK2 &= ~(1 << OCIE2A);
  TCCR2B |= (1 << CS22);
  TCCR2B &= ~((1 << CS21) | (1 << CS20));
}

void DUINO_Encoder::begin()
{
  TCNT2 = 256 - (int)((float)F_CPU * 0.000015625);
  TIMSK2 |= (1 << TOIE2);
}

void DUINO_Encoder::service()
{
  bool moved = false;
  unsigned long now = millis();

  // decelerate each tick
  acceleration_ -= ENC_ACCEL_DEC;
  if (acceleration_ & 0x8000)
  {
    acceleration_ = 0;
  }

  // check encoder
  int8_t curr = 0;
  if (digitalRead(pin_a_) == LOW)
  {
    curr = 3;
  }
  if (digitalRead(pin_b_) == LOW)
  {
    curr ^= 1;
  }
  int8_t diff = last_ - curr;
  if (diff & 1)
  {
    last_ = curr;
    delta_ += (diff & 2) - 1;
    moved = true;
  }

  // accelerate if moved
  if (moved)
  {
    if (acceleration_ <= (ENC_ACCEL_TOP - ENC_ACCEL_INC))
    {
      acceleration_ += ENC_ACCEL_INC;
    }
  }

  static uint16_t key_down_ticks = 0;
  static uint8_t double_click_ticks = 0;
  static unsigned long last_button_check = 0;

  // check button
  if ((now - last_button_check) >= ENC_BUTTONINTERVAL)
  { 
    last_button_check = now;
    
    if (digitalRead(pin_btn_) == LOW)
    {
      key_down_ticks++;
      if (key_down_ticks > (ENC_HOLDTIME / ENC_BUTTONINTERVAL))
      {
        button_ = Held;
      }
    }

    if (digitalRead(pin_btn_) == HIGH)
    {
      if (key_down_ticks /*> ENC_BUTTONINTERVAL*/)
      {
        if (button_ == Held)
        {
          button_ = Released;
          double_click_ticks = 0;
        }
        else
        {
          if (double_click_ticks > 1)
          {
            if (double_click_ticks < (ENC_DOUBLECLICKTIME / ENC_BUTTONINTERVAL))
            {
              button_ = DoubleClicked;
              double_click_ticks = 0;
            }
          }
          else
          {
            double_click_ticks = ENC_DOUBLECLICKTIME / ENC_BUTTONINTERVAL;
          }
        }
      }

      key_down_ticks = 0;
    }
  
    if (double_click_ticks > 0)
    {
      double_click_ticks--;
      if (--double_click_ticks == 0)
      {
        button_ = Clicked;
      }
    }
  }
}

int16_t DUINO_Encoder::get_value()
{
  int16_t val;

  // read delta
  cli();
  val = delta_;
  delta_ = val & 1;
  sei();
  val >>= 1;

  int16_t r = 0;
  int16_t accel = acceleration_ >> 8;

  if (val < 0)
  {
    r -= 1 + accel;
  }
  else if (val > 0)
  {
    r += 1 + accel;
  }

  return r;
}

DUINO_Encoder::Button DUINO_Encoder::get_button(void)
{
  Button r = button_;

  // reset
  if (button_ != Held)
  {
    button_ = Open;
  }

  return r;
}

DUINO_Encoder Encoder(9, 10, 12);

ISR(TIMER2_OVF_vect)
{
  Encoder.service();
}
