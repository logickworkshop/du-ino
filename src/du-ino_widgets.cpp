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

#include "du-ino_widgets.h"

DUINO_Widget::DUINO_Widget()
  : click_callback_(NULL)
  , double_click_callback_(NULL)
  , scroll_callback_(NULL)
{
}

void DUINO_Widget::on_click()
{
  if(click_callback_)
  {
    click_callback_();
  }
}

void DUINO_Widget::on_double_click()
{
  if(double_click_callback_)
  {
    double_click_callback_();
  }
}

void DUINO_Widget::on_scroll(int delta)
{
  if(scroll_callback_ && delta)
  {
    scroll_callback_(delta);
  }
}

void DUINO_Widget::attach_click_callback(void (*callback)())
{
  click_callback_ = callback;
}

void DUINO_Widget::attach_double_click_callback(void (*callback)())
{
  double_click_callback_ = callback;
}

void DUINO_Widget::attach_scroll_callback(void (*callback)(int))
{
  scroll_callback_ = callback;
}

DUINO_DisplayWidget::DUINO_DisplayWidget(uint8_t x, uint8_t y, uint8_t width, uint8_t height)
  : x_(x)
  , y_(y)
  , width_(width)
  , height_(height)
  , inverted_(false)
{
}

void DUINO_DisplayWidget::display()
{
  Display.display(x_, x_ + width_, y_ / 8, (y_ + width_) / 8);
}

void DUINO_DisplayWidget::invert(bool update_display)
{
  Display.fill_rect(x_, y_, width_, height_, DUINO_SH1106::Inverse);
  if (update_display)
  {
    display();
  }

  inverted_ = !inverted_;
}
