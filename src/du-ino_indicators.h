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

/** Jack indicator UI element. */
class DUINO_JackIndicator : public DUINO_DisplayObject
{
public:
  /**
   * Constructor.
   *
   * \param x Position x coordinate.
   * \param y Position y coordinate.
   */
  DUINO_JackIndicator(uint8_t x, uint8_t y);

  /**
   * Set the state of the indicator.
   *
   * \param state The state of the indicator (displays a center dot if true).
   */
  void set(bool state);

  /**
   * Toggle the state of the indicator.
   */
  void toggle();

  /**
   * Get the current state of the indicator.
   *
   * \return The current state of the indicator.
   */
  bool state() const { return state_; }

  virtual uint8_t width() const {return 7; }
  virtual uint8_t height() const { return 7; }

protected:
  void draw_jack();
  void draw_state();

  bool state_;
};

#endif // DUINO_INDICATORS_H_
