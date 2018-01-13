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
 * DU-INO Arduino Library - User Interface Module
 * Aaron Mavrinac <aaron@logick.ca>
 */

#include <EEPROM.h>

#include "du-ino_interface.h"

DUINO_Interface::DUINO_Interface()
  : display(new DUINO_SSD1306())
  , encoder(new DUINO_Encoder(9, 10, 12))
{

}

void DUINO_Interface::begin()
{
  static bool initialized = false;

  if(!initialized)
  {
    // initialize display
    display->begin();
    display->clear_display(); 
    display->display_all();

    // initialize encoder
    encoder->begin();

    setup();
    initialized = true;
  }
}

void DUINO_Interface::save_params(int address, uint8_t * start, int length)
{
  for(int i = 0; i < length; ++i)
  {
    EEPROM.write(address + i, *(start + i));
  }
}

void DUINO_Interface::load_params(int address, uint8_t * start, int length)
{
  for(int i = 0; i < length; ++i)
  {
    *(start + i) = EEPROM.read(address + i);
  }
}
