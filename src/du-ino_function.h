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

#define CI1         A0
#define CI2         A1
#define CI3         A2
#define CI4         A3

#define CO1         0
#define CO2         1
#define CO3         3
#define CO4         2

#define GT6         5
#define GT5         4
#define GT4         3
#define GT3         2
#define GT2         1
#define GT1         0

#define GT_MULTI    0x80
// e.g. gt_out(GT_MULTI | (1 << GT5) | (1 << GT1), true);
#define GT_ALL      0xBF

class DUINO_MCP4922;

class DUINO_Function {
 public:
  DUINO_Function(uint16_t sc);

  void begin();

  virtual void setup() {}
  virtual void loop() {}

  bool gt_read(uint8_t jack);
  void gt_out(uint8_t jack, bool on, bool trig = false);

  float cv_read(uint8_t jack);
  void cv_out(uint8_t jack, float value);
  void cv_hold(bool state);

  void gt_attach_interrupt(uint8_t jack, void (*isr)(void), int mode);
  void gt_detach_interrupt(uint8_t jack);

 private:
  DUINO_MCP4922 * dac[2];

  const uint16_t switch_config;   // 0b000000{S1 .. S10}
  const uint8_t out_mask;         // 0b00{GT6 .. GT1}
};

#endif // DUINO_FUNCTION_H_
