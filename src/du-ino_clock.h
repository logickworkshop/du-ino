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

/** Clock class. */
class DUINO_Clock {
 public:
  DUINO_Clock();

  void begin();

  /**
   * Set the period of the clock, in milliseconds.
   *
   * \param period The period in milliseconds.
   */
  void set_period(unsigned long period);

  /**
   * Set the beats per minute (quarter notes) of the clock.
   *
   * \param bpm Beats per minute (quarter notes).
   */
  void set_bpm(uint16_t bpm);

  /**
   * Set the swing delay of the 2 and 4 beats.
   *
   * \param swing Swing amount (0 to 6; 0 = 50%, 1 = 54%, and so on up to 6 = 74%).
   */
  void set_swing(uint8_t swing);

  /**
   * Set the clock divider.
   *
   * \param divider The clock divider value (1 to 64).
   */
  void set_divider(uint8_t divider);

  /**
   * Enable the external clock source.
   */
  void set_external();

  /**
   * Return the current state of the clock pulse.
   *
   * \return The current state of the clock pulse.
   */
  bool state() const { return state_; }

  /**
   * Return the current count, modulo 64, of the clock, before division.
   *
   * \return The current count.
   */
  uint8_t count() const { return count_; }

  /**
   * Check if the clock pulse has had a rising edge since the last check or reset.
   *
   * \return True if the clock pulse has had a rising edge.
   */
  bool retrigger();

  /**
   * Reset the clock state immediately.
   */
  void reset();

  /**
   * Attach a callback to be called when the clock state changes.
   */
  void attach_clock_callback(void (*callback)()) { clock_callback_ = callback; }

  /**
   * Attach a callback to be called when the clock switches from internal timer to external input.
   */
  void attach_external_callback(void (*callback)()) { external_callback_ = callback; }

  bool get_external() const { return external_; }
  unsigned long get_period() const { return period_; }
  uint8_t get_swing() const { return swing_; }
  uint8_t get_divider() const { return divider_; }

  /**
   * Callback method intended to be called by the external clock input jack ISR.
   *
   * \param jack_state The state of the external clock input jack on the ISR call.
   */
  void on_jack(bool jack_state);

  /**
   * Callback method called either by the internal timer ISR or the external on_jack() callback.
   */
  void on_clock();

  /**
   * Callback method called by the swing timer ISR.
   */
  void check_swing();

 protected:
  void update();
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
