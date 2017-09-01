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
 * DU-INO Arduino Library - Function Module
 * Aaron Mavrinac <aaron@logick.ca>
 */

#ifndef DUINO_FUNCTION_H_
#define DUINO_FUNCTION_H_

#include "Arduino.h"

class DUINO_MCP4922;

class DUINO_Function {
 public:
  DUINO_Function(uint16_t sc);

  virtual void setup() = 0;
  virtual void loop() = 0;

 private:
  uint8_t digital_read(uint8_t jack);

  float analog_read(uint8_t jack);

  void gate(uint8_t jack);
  void multi_gate(uint8_t jacks);
  void clear(uint8_t jack);
  void multi_clear(uint8_t jacks);
  void trig(uint8_t jack);
  void multi_trig(uint8_t jacks);

  void analog_write(uint8_t dac_pin, float value);

  DUINO_MCP4922 * dac[2];

  const uint16_t switch_config;   // 0b000000{S1 .. S10}
  const uint8_t out_mask;         // 0b00{J9 .. J14}
};

#endif // DUINO_FUNCTION_H_
