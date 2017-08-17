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
  : pin_a(a)
  , pin_b(b)
  , pin_btn(btn)
  , delta(0)
  , last(0)
  , acceleration(0)
  , button(Open)
{
  // configure pins for active-low operation
  pinMode(pin_a, INPUT_PULLUP);
  pinMode(pin_b, INPUT_PULLUP);
  pinMode(pin_btn, INPUT_PULLUP);

  // initialize state
  if (digitalRead(pin_a) == LOW)
    last = 3;
  if (digitalRead(pin_b) == LOW)
    last ^= 1;
}

void DUINO_Encoder::service()
{
  bool moved = false;
  unsigned long now = millis();

  // decelerate each tick
  acceleration -= ENC_ACCEL_DEC;
  if(acceleration & 0x8000)
      acceleration = 0;

  // check encoder
  int8_t curr = 0;
  if(digitalRead(pin_a) == LOW)
    curr = 3;
  if(digitalRead(pin_b) == LOW)
    curr ^= 1;
  int8_t diff = last - curr;
  if(diff & 1)
  {
    last = curr;
    delta += (diff & 2) - 1;
    moved = true;
  }

  // accelerate if moved
  if(moved)
    if(acceleration <= (ENC_ACCEL_TOP - ENC_ACCEL_INC))
      acceleration += ENC_ACCEL_INC;

  static uint16_t key_down_ticks = 0;
  static uint8_t double_click_ticks = 0;
  static unsigned long last_button_check = 0;

  // check button
  if((now - last_button_check) >= ENC_BUTTONINTERVAL)
  { 
    last_button_check = now;
    
    if (digitalRead(pin_btn) == LOW)
    {
      key_down_ticks++;
      if(key_down_ticks > (ENC_HOLDTIME / ENC_BUTTONINTERVAL))
        button = Held;
    }

    if (digitalRead(pin_btn) == HIGH)
    {
      if(key_down_ticks /*> ENC_BUTTONINTERVAL*/)
      {
        if(button == Held)
        {
          button = Released;
          double_click_ticks = 0;
        }
        else
        {
          if(double_click_ticks > 1)
          {
            if(double_click_ticks < (ENC_DOUBLECLICKTIME / ENC_BUTTONINTERVAL))
            {
              button = DoubleClicked;
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
  
    if(double_click_ticks > 0)
    {
      double_click_ticks--;
      if(--double_click_ticks == 0)
        button = Clicked;
    }
  }
}

int16_t DUINO_Encoder::get_value()
{
  int16_t val;

  // read delta
  cli();
  val = delta;
  delta = 0;
  sei();

  int16_t r = 0;
  int16_t accel = acceleration >> 8;

  if(val < 0)
    r -= 1 + accel;
  else if(val > 0)
    r += 1 + accel;

  return r;
}

DUINO_Encoder::Button DUINO_Encoder::get_button(void)
{
  Button r = button;

  // reset
  if (button != Held)
  {
    button = Open;
  }

  return r;
}
