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
 * DU-INO Arduino Library - Clock Module
 * Aaron Mavrinac <aaron@logick.ca>
 */

#ifndef DUINO_CLOCK_H_
#define DUINO_CLOCK_H_

#include "Arduino.h"

class DUINO_Clock {
 public:
  DUINO_Clock();

  void begin();

  void set_period(unsigned long period);
  void set_bpm(uint16_t bpm);
  void set_swing(uint8_t swing);
  void set_divider(uint8_t divider);
  void set_external();

  void on_jack(bool jack_state);
  void on_clock();

  bool state() const { return state_; }
  uint8_t count() const { return count_; }
  bool retrigger();

  void update();
  void reset();

  void attach_clock_callback(void (*callback)()) { clock_callback_ = callback; }
  void attach_external_callback(void (*callback)()) { external_callback_ = callback; }

  bool get_external() const { return external_; }
  unsigned long get_period() const { return period_; }
  uint8_t get_swing() const { return swing_; }
  uint8_t get_divider() const { return divider_; }

  void check_swing();

 protected:
  void toggle_state();

  void (*clock_callback_)();
  void (*external_callback_)();

  volatile bool state_, retrigger_flag_, swung_, external_;
  volatile int8_t count_;
  volatile unsigned long swung_ms_;

  unsigned long period_;
  uint8_t swing_, divider_, div_count_;
};

extern DUINO_Clock Clock;

#endif // DUINO_CLOCK_H_
