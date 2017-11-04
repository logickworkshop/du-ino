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
    display->display();

    setup();
    initialized = true;
  }
}

void DUINO_Interface::timer_isr()
{
  encoder->service();
}
