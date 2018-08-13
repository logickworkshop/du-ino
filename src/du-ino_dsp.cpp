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

#include "du-ino_dsp.h"

#define TWO_PI 6.28318530717958647693
#define UL_MAX 4294967295

DUINO_Filter::DUINO_Filter(DUINO_Filter::FilterType type, float frequency, float value)
  : ft_(type)
  , in_(value)
  , out_(value)
  , last_out_(value)
{
  set_frequency(frequency);
}

float DUINO_Filter::filter(float value)
{
  // get time elapsed since last input
  unsigned long current_time = micros();
  float elapsed;
  if (current_time > last_time_)
  {
    elapsed = (float)(current_time - last_time_);
  }
  else
  {
    elapsed = (float)(UL_MAX - last_time_ + current_time);
  }
  last_time_ = current_time;

  // filter input
  in_ = value;
  last_out_ = out_;
  float a = exp(-1.0 / (tau_ / elapsed));
  out_ = (1.0 - a) * in_ + a * last_out_;

  // return output
  switch (ft_)
  {
    case LowPass:
      return out_;
    case HighPass:
      return in_ - out_;
  }
}

void DUINO_Filter::set_frequency(float frequency)
{
  tau_ = 1e6 / (TWO_PI * frequency);
}

void DUINO_Filter::set_tau(float tau_s)
{
  // convert to microseconds
  tau_ = tau_s * 1e6;
}
