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

class DUINO_SSD1306;
class DUINO_Encoder;

class DUINO_Interface {
 public:
  static DUINO_Interface& get();

  void begin();

  void timer_isr();

 private:
  DUINO_Interface();

  DUINO_Interface(DUINO_Interface const&);
  void operator=(DUINO_Interface const&);

  DUINO_SSD1306 * display;
  DUINO_Encoder * encoder;
};

#endif // DUINO_INTERFACE_H_
