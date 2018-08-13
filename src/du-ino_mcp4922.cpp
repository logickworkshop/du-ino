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

#include <SPI.h>

#include "du-ino_mcp4922.h"

DUINO_MCP4922::DUINO_MCP4922(uint8_t ss, uint8_t ldac)
  : pin_ss_(ss)
  , pin_ldac_(ldac)
{
  // configure chip select for output
  pinMode(pin_ss_, OUTPUT);
  pinMode(pin_ldac_, OUTPUT);
}

void DUINO_MCP4922::begin()
{
  // hold chip deselect
  digitalWrite(pin_ss_, HIGH);

  // hold LDAC low
  digitalWrite(pin_ldac_, LOW);

  // configure SPI
  SPI.begin();
  SPI.setBitOrder(MSBFIRST);
  SPI.setDataMode(SPI_MODE0);
  SPI.setClockDivider(SPI_CLOCK_DIV2);
}

void DUINO_MCP4922::output(Channel channel, uint16_t data)
{
  // truncate to 12 bits
  data &= 0xfff;
  // add control bits
  data |= (channel << 15) | 0x7000;

  // chip select
  digitalWrite(pin_ss_, LOW);

  // send command
  SPI.transfer((data & 0xff00) >> 8);
  SPI.transfer(data & 0xff);

  // chip deselect
  digitalWrite(pin_ss_, HIGH);
}

void DUINO_MCP4922::hold(bool state)
{
  digitalWrite(pin_ldac_, state ? HIGH : LOW);
}
