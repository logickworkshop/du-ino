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

template <unsigned int N>
class DUINO_WidgetContainer : public DUINO_Widget
{
public:
  DUINO_WidgetContainer(Action t, unsigned int initial_selection = 0)
    : type_(t)
    , selected_(initial_selection)
  {
    for (unsigned int i = 0; i < N; ++i)
    {
      children_[i] = NULL;
    } 
  }

  virtual void invert(bool update_display = true)
  {
    if (children_[selected_])
    {
      children_[selected_]->invert(update_display);
    }
  }

  virtual bool inverted() const
  {
    if (children_[selected_])
    {
      return children_[selected_]->inverted();
    }
  }

  virtual void on_click()
  {
    switch (type_)
    {
      case Click:
        select_next();
        break;
      default:
        if (children_[selected_])
        {
          children_[selected_]->on_click();
        }
        break;
    }

    if(click_callback_)
    {
      click_callback_();
    }
  }

  virtual void on_double_click()
  {
    switch (type_)
    {
      case DoubleClick:
        select_next();
        break;
      default:
        if (children_[selected_])
        {
          children_[selected_]->on_double_click();
        }
        break;
    }

    if(double_click_callback_)
    {
      double_click_callback_();
    }
  }

  virtual void on_scroll(int delta)
  {
    if(delta == 0)
    {
      return;
    }

    switch (type_)
    {
      case Scroll:
        select_delta(delta);
        break;
      default:
        if (children_[selected_])
        {
          children_[selected_]->on_scroll(delta);
        }
        break;
    }

    if(scroll_callback_)
    {
      scroll_callback_(delta);
    }
  }

  void attach_child(DUINO_Widget * child, unsigned int position)
  {
    children_[position] = child;
  }

  void select(unsigned int selection)
  {
    invert_selected();
    if (selection < N)
    {
      selected_ = selection;
    }
    invert_selected();
  }

  void select_delta(int delta)
  {
    invert_selected();
    selected_ += delta;
    selected_ %= N;
    if (selected_ < 0)
    {
      selected_ += N;
    }
    invert_selected();
  }

  void select_prev()
  {
    invert_selected();
    selected_--;
    if (selected_ < 0)
    {
      selected_ = N;
    }
    invert_selected();
  }

  void select_next()
  {
    invert_selected();
    selected_++;
    selected_ %= N;
    invert_selected();
  }

  int selected() const { return selected_; }

protected:
  inline void invert_selected()
  {
    if (inverted() && children_[selected_])
    {
      children_[selected_]->invert();
    }
  }

  const Action type_;
  int selected_;
  DUINO_Widget * children_[N];
};
