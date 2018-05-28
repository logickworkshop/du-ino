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
#include "du-ino_sh1106.h"
#include "du-ino_encoder.h"

class DUINO_Interface {
 public:
  DUINO_Interface();

  void begin();

  virtual void setup() {}
  virtual void loop() {}

  void save_params(int address, uint8_t * start, int length);
  void load_params(int address, uint8_t * start, int length);

  DUINO_SH1106 * display;
  DUINO_Encoder * encoder;

 protected:
  bool saved;
};

#endif // DUINO_INTERFACE_H_
