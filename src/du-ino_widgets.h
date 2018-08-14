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

class DUINO_SH1106;

class DUINO_Widget
{
public:
  DUINO_Widget() { }

  virtual void invert(bool update_display = true) { }

  virtual void on_click() { }
  virtual void on_double_click() { }
  virtual void on_scroll(int delta) { }
};

class DUINO_DisplayWidget
{
public:
  DUINO_DisplayWidget(uint16_t x, uint16_t y, uint16_t width, uint16_t height, DUINO_SH1106 * display);

  virtual void invert(bool update_display = true);

  uint16_t x() { return x_; }
  uint16_t y() { return y_; }
  uint16_t width() { return width_; }
  uint16_t height() { return height_; }

  virtual void on_click();
  virtual void on_double_click();
  virtual void on_scroll(int delta);

  void attach_click_callback(void (*callback)());
  void attach_double_click_callback(void (*callback)());
  void attach_scroll_callback(void (*callback)(int));

protected:
  uint16_t x_, y_, width_, height_;
  DUINO_SH1106 * display_;

  void (*click_callback_)();
  void (*double_click_callback_)();
  void (*scroll_callback_)(int);
};

template <unsigned int N>
class DUINO_WidgetContainer : public DUINO_Widget
{
public:
  enum ContainerType
  {
    Click,
    DoubleClick,
    Scroll
  };

  DUINO_WidgetContainer(ContainerType t, unsigned int initial_selection = 0)
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
  }

  virtual void on_scroll(int delta)
  {
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

protected:
  inline void invert_selected()
  {
    if (children_[selected_])
    {
      children_[selected_]->invert();
    }
  }

  const ContainerType type_;
  int selected_;
  DUINO_Widget * children_[N];
};
