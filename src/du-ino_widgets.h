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

class DUINO_Widget
{
public:
  enum Action
  {
    Click,
    DoubleClick,
    Scroll
  };

  DUINO_Widget();

  virtual void invert(bool update_display = true) { }
  virtual bool inverted() const { return false; }

  virtual void on_click();
  virtual void on_double_click();
  virtual void on_scroll(int delta);

  void attach_click_callback(void (*callback)());
  void attach_double_click_callback(void (*callback)());
  void attach_scroll_callback(void (*callback)(int));

protected:
  void (*click_callback_)();
  void (*double_click_callback_)();
  void (*scroll_callback_)(int);
};

class DUINO_DisplayWidget : public DUINO_Widget
{
public:
  DUINO_DisplayWidget(uint8_t x, uint8_t y, uint8_t width, uint8_t height);

  void display();

  virtual void invert(bool update_display = true);
  virtual bool inverted() const { return inverted_; }

  uint8_t x() const { return x_; }
  uint8_t y() const { return y_; }
  uint8_t width() const { return width_; }
  uint8_t height() const { return height_; }

protected:
  const uint8_t x_, y_, width_, height_;
  bool inverted_;
};

template <uint8_t N>
class DUINO_WidgetArray : public DUINO_Widget
{
public:
  DUINO_WidgetArray(int t, uint8_t initial_selection = 0)
    : type_(t)
    , selected_(initial_selection)
  { }

  void select(uint8_t selection)
  {
    const bool show = inverted();
    if(show)
    {
      invert_selected();
    }
    if (selection < N)
    {
      selected_ = selection;
    }
    if(show)
    {
      invert_selected();
    }
  }

  void select_delta(int delta)
  {
    const bool show = inverted();
    if(show)
    {
      invert_selected();
    }
    selected_ += delta;
    selected_ %= N;
    if (selected_ < 0)
    {
      selected_ += N;
    }
    if(show)
    {
      invert_selected();
    }
  }

  void select_prev()
  {
    const bool show = inverted();
    if(show)
    {
      invert_selected();
    }
    selected_--;
    if (selected_ < 0)
    {
      selected_ = N;
    }
    if(show)
    {
      invert_selected();
    }
  }

  void select_next()
  {
    const bool show = inverted();
    if(show)
    {
      invert_selected();
    }
    selected_++;
    selected_ %= N;
    if(show)
    {
      invert_selected();
    }
  }

  int selected() const { return selected_; }

protected:
  virtual void invert_selected() = 0;

  const Action type_;
  int selected_;
};

template <uint8_t N>
class DUINO_MultiDisplayWidget : public DUINO_WidgetArray<N>
{
public:
  DUINO_MultiDisplayWidget(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t step, bool vertical,
    DUINO_Widget::Action t, uint8_t initial_selection = 0)
    : x_(x)
    , y_(y)
    , width_(width)
    , height_(height)
    , step_(step)
    , vertical_(vertical)
    , inverted_(false)
    , DUINO_WidgetArray<N>(t, initial_selection)
  { }

  void display()
  {
    Display.display(x(this->selected_), x(this->selected_) + width_ - 1, y(this->selected_) >> 3,
        (y(this->selected_) + height_ - 1) >> 3);
  }

  virtual void invert(bool update_display = true)
  {
    Display.fill_rect(x(this->selected_), y(this->selected_), width_, height_, DUINO_SH1106::Inverse);
    if (update_display)
    {
      display();
    }

    inverted_ = !inverted_;
  }

  virtual bool inverted() const { return inverted_; }

  virtual void on_click()
  {
    switch (this->type_)
    {
      case DUINO_Widget::Click:
        select_next();
        break;
      default:
        if(click_callback_i_)
        {
          click_callback_i_(this->selected_);
        }
        break;
    }

    if(this->click_callback_)
    {
      this->click_callback_();
    }
  }

  virtual void on_double_click()
  {
    switch (this->type_)
    {
      case DUINO_Widget::DoubleClick:
        select_next();
        break;
      default:
        if(double_click_callback_i_)
        {
          double_click_callback_i_(this->selected_);
        }
        break;
    }

    if(this->double_click_callback_)
    {
      this->double_click_callback_();
    }
  }

  virtual void on_scroll(int delta)
  {
    if(delta == 0)
    {
      return;
    }

    switch (this->type_)
    {
      case DUINO_Widget::Scroll:
        select_delta(delta);
        break;
      default:
        if(scroll_callback_i_)
        {
          scroll_callback_i_(this->selected_, delta);
        }
        break;
    }

    if(this->scroll_callback_)
    {
      this->scroll_callback_(delta);
    }
  }

  void attach_click_callback_i(void (*callback)(uint8_t)) { click_callback_i_ = callback; }
  void attach_double_click_callback_i(void (*callback)(uint8_t)) { double_click_callback_i_ = callback; }
  void attach_scroll_callback_i(void (*callback)(uint8_t, int)) { scroll_callback_i_ = callback; }

  uint8_t x(uint8_t i) const { return x_ + (vertical_ ? 0 : i * step_); }
  uint8_t y(uint8_t i) const { return y_ + (vertical_ ? i * step_ : 0); }
  uint8_t width() const { return width_; }
  uint8_t height() const { return height_; }

protected:
  virtual void invert_selected()
  {
    invert();
  }

  const uint8_t x_, y_, width_, height_, step_;
  const bool vertical_;
  bool inverted_;

  void (*click_callback_i_)(uint8_t);
  void (*double_click_callback_i_)(uint8_t);
  void (*scroll_callback_i_)(uint8_t, int);
};

template <uint8_t N>
class DUINO_WidgetContainer : public DUINO_WidgetArray<N>
{
public:
  DUINO_WidgetContainer(DUINO_Widget::Action t, uint8_t initial_selection = 0)
    : DUINO_WidgetArray<N>(t, initial_selection)
  {
    for (uint8_t i = 0; i < N; ++i)
    {
      children_[i] = NULL;
    } 
  }

  virtual void invert(bool update_display = true)
  {
    if (children_[this->selected_])
    {
      children_[this->selected_]->invert(update_display);
    }
  }

  virtual bool inverted() const
  {
    if (children_[this->selected_])
    {
      return children_[this->selected_]->inverted();
    }
  }

  virtual void on_click()
  {
    switch (this->type_)
    {
      case DUINO_Widget::Click:
        select_next();
        break;
      default:
        if (children_[this->selected_])
        {
          children_[this->selected_]->on_click();
        }
        break;
    }

    if(this->click_callback_)
    {
      this->click_callback_();
    }
  }

  virtual void on_double_click()
  {
    switch (this->type_)
    {
      case DUINO_Widget::DoubleClick:
        select_next();
        break;
      default:
        if (children_[this->selected_])
        {
          children_[this->selected_]->on_double_click();
        }
        break;
    }

    if(this->double_click_callback_)
    {
      this->double_click_callback_();
    }
  }

  virtual void on_scroll(int delta)
  {
    if(delta == 0)
    {
      return;
    }

    switch (this->type_)
    {
      case DUINO_Widget::Scroll:
        select_delta(delta);
        break;
      default:
        if (children_[this->selected_])
        {
          children_[this->selected_]->on_scroll(delta);
        }
        break;
    }

    if(this->scroll_callback_)
    {
      this->scroll_callback_(delta);
    }
  }

  void attach_child(DUINO_Widget * child, uint8_t position)
  {
    children_[position] = child;
  }

protected:
  virtual void invert_selected()
  {
    if (children_[this->selected_])
    {
      children_[this->selected_]->invert();
    }
  }

  DUINO_Widget * children_[N];
};
