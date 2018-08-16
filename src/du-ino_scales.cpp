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
 * DU-INO Arduino Library - Musical Scale Module
 * Aaron Mavrinac <aaron@logick.ca>
 */

#include "du-ino_scales.h"

uint16_t get_scale_by_id(int id)
{
  if (id >= 0 && id < N_SCALES)
  {
    return (uint16_t)pgm_read_byte(&scales[id * 5]) << 8 | (uint16_t)pgm_read_byte(&scales[id * 5 + 1]);
  }

  return 0;
}

int get_id_from_scale(uint16_t scale)
{
  unsigned char scale_low = scale & 0xFF;
  unsigned char scale_high = scale >> 8;

  for (int id = 0; id < N_SCALES; ++id)
  {
    if (scale_high == pgm_read_byte(&scales[id * 5]) && scale_low == pgm_read_byte(&scales[id * 5 + 1]))
    {
      return id;
    }
  }

  return -1;
}
