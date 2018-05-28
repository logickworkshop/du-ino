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

#define GT1         0
#define GT2         1
#define GT3         2
#define GT4         3


#define GT_MULTI    0x80
// e.g. gt_out(GT_MULTI | (1 << GT4) | (1 << GT1), true);
#define GT_ALL      0x0F

class DUINO_MCP4922;

class DUINO_Function {
 public:
  DUINO_Function(uint8_t sc);

  void begin();

  virtual void setup() {}
  virtual void loop() {}

  bool gt_read(uint8_t jack);
  bool gt_read_debounce(uint8_t jack);
  void gt_out(uint8_t jack, bool on, bool trig = false);

  float cv_read(uint8_t jack);
  void cv_out(uint8_t jack, float value);
  void cv_hold(bool state);

  void gt_attach_interrupt(uint8_t jack, void (*isr)(void), int mode);
  void gt_detach_interrupt(uint8_t jack);

  void set_switch_config(uint8_t sc);

 private:
  DUINO_MCP4922 * dac[2];

  uint8_t switch_config;    // 0b{SC4 .. SC1}{SG4 .. SG1}
};

#endif // DUINO_FUNCTION_H_


/* SWITCH   NC (0)        NO (1)
 * ------   ----------    -----------
 * SG1      GT1 output    GT1 input
 * SG2      GT2 output    GT2 input
 * SG3      GT3 output    GT3 input
 * SG4      GT4 output    GT4 input
 * SC1      AD1+ = CI1    AD1+ = DAC1
 * SC2      AD1- = CI2    AD1- = DAC2
 * SC3      AD2+ = CI3    AD2+ = DAC3
 * SC4      AD2- = CI4    AD2- = DAC4
 */
