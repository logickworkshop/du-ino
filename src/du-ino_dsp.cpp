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

#include <du-ino_dsp.h>

#define TWO_PI 6.28318530717958647693
#define UL_MAX 4294967295

DUINO_Filter::DUINO_Filter(DUINO_Filter::FilterType type, float frequency, float value)
  : ft(type)
  , in(value)
  , out(value)
  , last_out(value)
{
  set_frequency(frequency);
}

float DUINO_Filter::filter(float value)
{
  // get time elapsed since last input
  unsigned long current_time = micros();
  float elapsed;
  if(current_time > last_time)
  {
    elapsed = (float)(current_time - last_time);
  }
  else
  {
    elapsed = (float)(UL_MAX - last_time + current_time);
  }
  last_time = current_time;

  // filter input
  in = value;
  last_out = out;
  float a = exp(-1.0 / (tau / elapsed));
  out = (1.0 - a) * in + a * last_out;

  // return output
  switch(ft)
  {
    case LowPass:
      return out;
    case HighPass:
      return in - out;
  }
}

void DUINO_Filter::set_frequency(float frequency)
{
  tau = 1e6 / (TWO_PI * frequency);
}
