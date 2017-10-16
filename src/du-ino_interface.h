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
 * DU-INO Arduino Library - User Interface Module
 * Aaron Mavrinac <aaron@logick.ca>
 */

#ifndef DUINO_INTERFACE_H_
#define DUINO_INTERFACE_H_

#include "Arduino.h"
#include "du-ino_ssd1306.h"
#include "du-ino_encoder.h"

class DUINO_Interface {
 public:
  DUINO_Interface();

  void begin();
  void timer_isr();

  virtual void setup() {}
  virtual void loop() {}

  DUINO_SSD1306 * display;
  DUINO_Encoder * encoder;
};

#endif // DUINO_INTERFACE_H_
