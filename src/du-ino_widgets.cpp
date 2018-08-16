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

void DUINO_Widget::draw_invert(uint8_t x, uint8_t y, uint8_t width, uint8_t height, InvertStyle style)
{
  switch(style)
  {
    case Full:
      Display.fill_rect(x, y, width, height, DUINO_SH1106::Inverse);
      break;
    case Box:
      Display.draw_vline(x, y, height, DUINO_SH1106::Inverse);
      Display.draw_vline(x + width - 1, y, height, DUINO_SH1106::Inverse);
      Display.draw_hline(x + 1, y, width - 2, DUINO_SH1106::Inverse);
      Display.draw_hline(x + 1, y + height - 1, width - 2, DUINO_SH1106::Inverse);
      break;
    case DottedBox:
      for(uint8_t i = 0; i < width; i += 2)
      {
        Display.draw_pixel(x + i, y, DUINO_SH1106::Inverse);
        Display.draw_pixel(x + i, y + height - 1, DUINO_SH1106::Inverse);
      }
      for(uint8_t i = 2; i < height - 2; i += 2)
      {
        Display.draw_pixel(x, y + i, DUINO_SH1106::Inverse);
        Display.draw_pixel(x + width - 1, y + i, DUINO_SH1106::Inverse);
      }
      break;
    case Corners:
      Display.draw_vline(x, y, 2, DUINO_SH1106::Inverse);
      Display.draw_pixel(x + 1, y, DUINO_SH1106::Inverse);
      Display.draw_vline(x + width - 1, y, 2, DUINO_SH1106::Inverse);
      Display.draw_pixel(x + width - 2, y, DUINO_SH1106::Inverse);
      Display.draw_vline(x, y + height - 2, 2, DUINO_SH1106::Inverse);
      Display.draw_pixel(x, y + height - 1, DUINO_SH1106::Inverse);
      Display.draw_vline(x + width - 1, y + height - 2, 2, DUINO_SH1106::Inverse);
      Display.draw_pixel(x + width - 2, y + height - 1, DUINO_SH1106::Inverse);
      break;
    }
}

DUINO_DisplayWidget::DUINO_DisplayWidget(uint8_t x, uint8_t y, uint8_t width, uint8_t height,
    DUINO_Widget::InvertStyle style)
  : x_(x)
  , y_(y)
  , width_(width)
  , height_(height)
  , style_(style)
  , inverted_(false)
{
}

void DUINO_DisplayWidget::display()
{
  Display.display(x_, x_ + width_, y_ / 8, (y_ + width_) / 8);
}

void DUINO_DisplayWidget::invert(bool update_display)
{
  draw_invert(x_, y_, width_, height_, style_);

  if (update_display)
  {
    display();
  }

  inverted_ = !inverted_;
}
