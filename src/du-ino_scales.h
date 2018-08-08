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

#ifndef DUINO_SCALES_H_
#define DUINO_SCALES_H_

#include <avr/pgmspace.h>

#define N_SCALES 31

static const unsigned char scales[] PROGMEM = {
  0b1111, 0b11111111, 'C', 'H', 'R',  // Chromatic
  0b1010, 0b10110101, 'I', 'O', 'N',  // Ionian
  0b0110, 0b10101101, 'D', 'O', 'R',  // Dorian
  0b0101, 0b10101011, 'P', 'H', 'R',  // Phrygian
  0b1010, 0b11010101, 'L', 'Y', 'D',  // Lydian
  0b0110, 0b10110101, 'M', 'X', 'L',  // Mixolydian
  0b0101, 0b10101101, 'A', 'E', 'O',  // Aeolian
  0b0101, 0b01101011, 'L', 'O', 'C',  // Locrian
  0b0110, 0b10011001, 'M', 'A', 'B',  // Major Blues
  0b0100, 0b11101001, 'M', 'I', 'B',  // Minor Blues
  0b1011, 0b01101101, 'D', 'I', 'M',  // Diminish
  0b0110, 0b11011011, 'C', 'D', 'M',  // Combination Diminish
  0b0010, 0b10010101, 'M', 'A', 'P',  // Major Pentatonic
  0b0100, 0b10101001, 'M', 'I', 'P',  // Minor Pentatonic
  0b1001, 0b10110011, 'R', 'G', '1',  // Raga Bhairav
  0b1010, 0b11010011, 'R', 'G', '2',  // Raga Gamanasrama
  0b1001, 0b11101011, 'R', 'G', '3',  // Raga Todi
  0b0101, 0b10111011, 'S', 'P', 'N',  // Spanish Scale
  0b1001, 0b11001101, 'G', 'Y', 'P',  // Gypsy Scale
  0b0101, 0b01110101, 'A', 'R', 'B',  // Arabian Scale
  0b0100, 0b10100101, 'E', 'G', 'Y',  // Egyptian Scale
  0b0010, 0b10001101, 'H', 'W', 'I',  // Hawaiian Scale
  0b0001, 0b10001011, 'P', 'L', 'G',  // Bali Island Pelog
  0b0001, 0b10100011, 'J', 'P', 'N',  // Japanese Miyakobushi
  0b1000, 0b10110001, 'R', 'K', 'Y',  // Ryukyu Scale
  0b0101, 0b01010101, 'W', 'H', 'L',  // Wholetone
  0b0010, 0b01001001, 'M', 'I', '3',  // Minor 3rd Interval
  0b0001, 0b00010001, '3', 'R', 'D',  // 3rd Interval
  0b0100, 0b00100001, '4', 'T', 'H',  // 4th Interval
  0b0000, 0b10000001, '5', 'T', 'H',  // 5th Interval
  0b0000, 0b00000001, 'O', 'C', 'T'   // Octave Interval
};

int scaleID(uint16_t scale);

#endif // DUINO_SCALES_H_
