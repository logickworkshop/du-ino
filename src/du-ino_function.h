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

enum Jack {
  GT1 = 0,
  GT2 = 1,
  GT3 = 2,
  GT4 = 3,
  CO1 = 4,
  CO2 = 5,
  CO3 = 7,
  CO4 = 6,
  CI1 = 8,
  CI2 = 9,
  CI3 = 10,
  CI4 = 11
};

class DUINO_MCP4922;

class DUINO_Function {
 public:
  DUINO_Function(uint8_t sc);

  void begin();

  virtual void setup() {}
  virtual void loop() {}

  bool gt_read(Jack jack);
  bool gt_read_debounce(Jack jack);
  void gt_out(Jack jack, bool on, bool trig = false);
  void gt_out_multi(uint8_t jacks, bool on, bool trig = false);

  float cv_read(Jack jack);
  void cv_out(Jack jack, float value);
  void cv_hold(bool state);

  void gt_attach_interrupt(Jack jack, void (*isr)(void), int mode);
  void gt_detach_interrupt(Jack jack);

  void set_switch_config(uint8_t sc);

 private:
  inline float cv_analog_read(uint8_t pin);

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
