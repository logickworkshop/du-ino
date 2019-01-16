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

/* This module is based in part upon Karl Pitrich's ClickEncoder [1] Arduino
 * library.
 * 
 * [1]: https://github.com/0xPIT/encoder
 */


#ifndef DUINO_ENCODER_H_
#define DUINO_ENCODER_H_

#include "Arduino.h"

/** Pushbutton encoder controller class. */
class DUINO_Encoder
{
public:
  enum Button {
    Open,
    Closed,
    Pressed,
    Held,
    Released,
    Clicked,
    DoubleClicked
  };

  /**
   * Constructor.
   *
   * \param a Address of pin A.
   * \param b Address of pin B.
   * \param btn Address of pushbutton pin.
   */
  DUINO_Encoder(uint8_t a, uint8_t b, uint8_t btn);

  /**
   * Initialize the pushbutton encoder controller.
   */
  void begin();

  /**
   * Service outstanding events (called by the tick timer ISR).
   */
  void service(void);

  /**
   * Get the current delta value (with acceleration) of the encoder.
   *
   * \return The directional delta value of the encoder since last read.
   */
  int16_t get_value(void);

  /**
   * Get the current state of the pushbutton (see Button enum for states).
   *
   * \return The current state of the pushbutton, with respect to last read.
   */
  Button get_button(void);

private:
  const uint8_t pin_a_, pin_b_, pin_btn_;

  volatile int16_t delta_, last_;
  volatile uint16_t acceleration_;
  volatile Button button_;
};

extern DUINO_Encoder Encoder;

#endif // DUINO_ENCODER_H_
