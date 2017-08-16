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

class DUINO_Encoder {
 public:
  DUINO_Encoder(uint8_t a, uint8_t b, uint8_t btn);

 private:
  const uint8_t pin_a;
  const uint8_t pin_b;
  const uint8_t pin_btn;
};

#endif // DUINO_ENCODER_H_