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
 * DU-INO Arduino Library - Parameter Save Widget Module
 * Aaron Mavrinac <aaron@logick.ca>
 */

#ifndef DUINO_SAVE_H_
#define DUINO_SAVE_H_

#include "Arduino.h"
#include <EEPROM.h>
#include "du-ino_widgets.h"

template <typename P>
class DUINO_SaveWidget : public DUINO_DisplayWidget
{
public:
  union Parameters
  {
    P vals;
    uint8_t bytes[sizeof(P)];
  };

  DUINO_SaveWidget(uint8_t x, uint8_t y, int address = 0)
    : address_(address)
    , saved_(false)
    , DUINO_DisplayWidget(x, y, 7, 7, DUINO_Widget::Full) { }

  virtual void on_click()
  {
    save_params();
    display();

    if(click_callback_)
    {
      click_callback_();
    }
  }

  void save_params()
  {
    if (saved_)
    {
      return;
    }

    for (int i = 0; i < sizeof(P); ++i)
    {
      EEPROM.write(address_ + i, *(params.bytes + i));
    }

    mark_saved();
  }

  void load_params()
  {
    for (int i = 0; i < sizeof(P); ++i)
    {
      *(params.bytes + i) = EEPROM.read(address_ + i);
    }

    mark_saved();
  }

  void mark_changed()
  {
    saved_ = false;
    Display.fill_rect(x_ + 2, y_ + 2, 3, 3, inverted() ? DUINO_SH1106::White : DUINO_SH1106::Black);
  }

  Parameters params;

protected:
  void mark_saved()
  {
    saved_ = true;
    Display.fill_rect(x_ + 2, y_ + 2, 3, 3, inverted() ? DUINO_SH1106::Black : DUINO_SH1106::White);
  }

  const int address_;
  bool saved_;
};

#endif // DUINO_SAVE_H_
