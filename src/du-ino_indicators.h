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

#ifndef DUINO_INDICATORS_H_
#define DUINO_INDICATORS_H_

#include "Arduino.h"
#include "du-ino_widgets.h"

class DUINO_JackIndicator : public DUINO_DisplayObject
{
public:
  DUINO_JackIndicator(uint8_t x, uint8_t y);

  void set(bool state);
  void toggle();

  bool state() const { return state_; }

  virtual uint8_t width() const {return 7; }
  virtual uint8_t height() const { return 7; }

protected:
  void draw_jack();
  void draw_state();

  bool state_;
};

#endif // DUINO_INDICATORS_H_
