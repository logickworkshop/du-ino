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

class DUINO_Filter
{
public:
  enum FilterType
  {
    LowPass,
    HighPass
  };

  DUINO_Filter(FilterType type, float frequency, float value);

  float filter(float value);

  void set_frequency(float frequency);
  void set_tau(float tau_s);

private:
  FilterType ft_;
  float tau_;
  float in_, out_, last_out_;
  unsigned long last_time_;
};

#endif // DUINO_DSP_H_
