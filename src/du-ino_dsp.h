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
 * DU-INO Arduino Library - Digital Signal Processing Module
 * Aaron Mavrinac <aaron@logick.ca>
 */

#ifndef DUINO_DSP_H_
#define DUINO_DSP_H_

#include "Arduino.h"

/** DSP filter class. */
class DUINO_Filter
{
public:
  enum FilterType
  {
    LowPass,
    HighPass
  };

  /**
   * Constructor.
   * 
   * \param type Filter type (low-pass or high-pass).
   * \param frequency Filter cutoff frequency.
   * \param value Initial signal value.
   */
  DUINO_Filter(FilterType type, float frequency, float value);

  /**
   * Update the signal value and return the filtered output.
   * 
   * \param value The new input signal value.
   * \return The filtered output signal value.
   */
  float filter(float value);

  /**
   * Set the cutoff frequency of the filter.
   *
   * \param frequency Filter cutoff frequency.
   */
  void set_frequency(float frequency);

  /**
   * Set the time constant of the filter. (Inversely proportional to cutoff frequency.)
   *
   * \param tau_s Time constant, in seconds.
   */
  void set_tau(float tau_s);

private:
  FilterType ft_;
  float tau_;
  float in_, out_, last_out_;
  unsigned long last_time_;
};

#endif // DUINO_DSP_H_
