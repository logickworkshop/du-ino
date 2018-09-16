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
 * DU-INO Arduino Library - Graphic Indicators Module
 * Aaron Mavrinac <aaron@logick.ca>
 */

#include <avr/pgmspace.h>
#include "du-ino_indicators.h"

static const unsigned char icons[] PROGMEM =
{
  0x1c, 0x22, 0x41, 0x41, 0x41, 0x22, 0x1c   // jack 
};

DUINO_JackIndicator::DUINO_JackIndicator(uint8_t x, uint8_t y)
  : state_(false)
  , DUINO_DisplayObject(x, y)
{
  draw_jack();
}

void DUINO_JackIndicator::set(bool state)
{
  state_ = state;
  draw_state();
}

void DUINO_JackIndicator::toggle()
{
  set(!state());
}

void DUINO_JackIndicator::draw_jack()
{
  Display.draw_bitmap_7(x(), y(), icons, 0, DUINO_SH1106::White);
}

void DUINO_JackIndicator::draw_state()
{
  Display.fill_rect(x() + 2, y() + 2, 3, 3, state_ ? DUINO_SH1106::White : DUINO_SH1106::Black);
}
