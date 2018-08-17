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
  void set_external();

  void on_jack(bool jack_state);
  void on_clock(bool jack_state = false);

  bool state() const { return state_; }
  bool retrigger();

  void update();

  void attach_clock_callback(void (*callback)()) { clock_callback_ = callback; }
  void attach_external_callback(void (*callback)()) { external_callback_ = callback; }

  bool get_external() const { return external_; }
  unsigned long get_period() const { return period_; }

 protected:
  void (*clock_callback_)();
  void (*external_callback_)();

  volatile bool state_;
  volatile bool retrigger_flag_;
  volatile bool external_;

  unsigned long period_;
};

extern DUINO_Clock Clock;

#endif // DUINO_CLOCK_H_
