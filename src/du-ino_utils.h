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
 * DU-INO Arduino Library - Utilities Module
 * Aaron Mavrinac <aaron@logick.ca>
 */

#ifndef DUINO_UTILS_H_
#define DUINO_UTILS_H_

#include "Arduino.h"

/**
 * Clamp a value between given minimum and maximum allowable values.
 *
 * \param value The input value.
 * \param value_min The minimum allowable value.
 * \param value_max The maximum allowable value.
 * \return The clamped output value.
 */
template <typename T>
T clamp(T value, T value_min, T value_max)
{
  return (value < value_min) ? value_min : (value_max < value) ? value_max : value;
}

/**
 * Adjust a value within given minimum and maximum allowable values, and return whether the value has changed.
 *
 * \param value The value to adjust.
 * \param delta The amount by which to adjust the value.
 * \param value_min The minimum allowable value.
 * \param value_max The maximum allowable value.
 * \return True if the value has changed.
 */
template <typename T>
bool adjust(T& value, T delta, T value_min, T value_max)
{
  const T initial_value = value;
  value += delta;
  value = clamp<T>(value, value_min, value_max);
  return value != initial_value;
}

#endif  // DUINO_UTILS_H_
