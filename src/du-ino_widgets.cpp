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
 * DU-INO Arduino Library - Interface Widget Library
 * Aaron Mavrinac <aaron@logick.ca>
 */

#include "du-ino_sh1106.h"
#include "du-ino_widgets.h"

DUINO_DisplayWidget::DUINO_DisplayWidget(uint16_t x, uint16_t y, uint16_t width, uint16_t height,
    DUINO_SH1106 * display)
  : x_(x)
  , y_(y)
  , width_(width)
  , height_(height)
  , display_(display)
  , click_callback_(NULL)
  , double_click_callback_(NULL)
  , scroll_callback_(NULL)
{
}

void DUINO_DisplayWidget::invert(bool update_display)
{
  display_->fill_rect(x_, y_, width_, height_, DUINO_SH1106::Inverse);
  if (update_display && display_)
  {
    display_->display(x_, x_ + width_, y_ / 8, (y_ + width_) / 8);
  }
}

void DUINO_DisplayWidget::on_click()
{
  click_callback_();
}

void DUINO_DisplayWidget::on_double_click()
{
  double_click_callback_();
}

void DUINO_DisplayWidget::on_scroll(int delta)
{
  scroll_callback_(delta);
}

void DUINO_DisplayWidget::attach_click_callback(void (*callback)())
{
  click_callback_ = callback;
}

void DUINO_DisplayWidget::attach_double_click_callback(void (*callback)())
{
  double_click_callback_ = callback;
}

void DUINO_DisplayWidget::attach_scroll_callback(void (*callback)(int))
{
  scroll_callback_ = callback;
}
