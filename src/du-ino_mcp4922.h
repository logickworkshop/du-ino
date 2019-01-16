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
 * DU-INO Arduino Library - Microchip MCP4922 SPI Dual 12-Bit DAC Module
 * Aaron Mavrinac <aaron@logick.ca>
 */

#ifndef DUINO_MCP4922_H_
#define DUINO_MCP4922_H_

#include "Arduino.h"

/** MCP4922 DAC driver class. */
class DUINO_MCP4922 {
public:
  enum Channel
  {
    A = 0,
    B = 1
  };

  /**
   * Constructor.
   *
   * \param ss Address of the chip select pin for this device.
   * \param ldac Address of the LDAC (hold) pin for this device.
   */
  DUINO_MCP4922(uint8_t ss, uint8_t ldac);

  /**
   * Initialize the MCP4922 DAC device.
   */
  void begin();

  /**
   * Output the specified digital value to the specified channel.
   *
   * \param channel The output channel (A or B).
   * \param data The raw digital data value.
   */
  void output(Channel channel, uint16_t data);

  /**
   * Hold or release the current output value of both channels.
   *
   * \param state If true, hold the current value on both channels until called with false.
   */
  void hold(bool state);

private:
  const uint8_t pin_ss_;
  const uint8_t pin_ldac_;
};

#endif // DUINO_FUNCTION_H_
