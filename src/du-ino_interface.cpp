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

#include <TimerOne.h>

#include "du-ino_ssd1306.h"
#include "du-ino_encoder.h"
#include "du-ino_interface.h"

void DUINO_timer_isr()
{
  DUINO_Interface::get().timer_isr();
}

DUINO_Interface & DUINO_Interface::get()
{
  static DUINO_Interface interface;
  static bool initialized = false;

  if(!initialized)
  {
    Timer1.initialize(1000);
    Timer1.attachInterrupt(DUINO_timer_isr);
    initialized = true;
  }

  return interface;
}

DUINO_Interface::DUINO_Interface()
  : display(new DUINO_SSD1306())
  , encoder(new DUINO_Encoder(9, 10, 8))
{

}

void DUINO_Interface::begin()
{
  // initialize display
  display->begin();
  display->clear_display(); 
  display->display();
}

void DUINO_Interface::timer_isr()
{
  encoder->service();
}
