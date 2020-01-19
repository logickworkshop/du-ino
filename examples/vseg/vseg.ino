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
 * DU-INO Prophet VS Envelope Generator
 * Aaron Mavrinac <aaron@logick.ca>
 *
 * JACK    FUNCTION
 * ----    --------
 * GT1 O -
 * GT2 O -
 * GT3 I - gate in
 * GT4 I - retrigger in
 * CI1   -
 * CI2   -
 * CI3   - VCA audio in
 * CI4   -
 * OFFST -
 * CO1   - 10V envelope out
 * CO2   -
 * CO3   - 5V envelope out
 * CO4   -
 * FNCTN - VCA audio out
 *
 * SWITCH CONFIGURATION
 * --------------------
 * SG2    [_][_]    SG1
 * SG4    [^][^]    SG3
 * SC2    [^][^]    SC1
 * SC4    [^][_]    SC3
 */

#include <du-ino_function.h>
#include <du-ino_dsp.h>
#include <du-ino_widgets.h>
#include <du-ino_indicators.h>
#include <du-ino_save.h>
#include <du-ino_utils.h>
#include <avr/pgmspace.h>

#define ENV_RELEASE_COEFF     -2.0
#define ENV_RELEASE_HOLD      3

#define LEVEL_MAX 99
#define RATE_MAX 96
#define LOOP_MIN -1
#define LOOP_MAX 5
#define REPEAT_MIN 0
#define REPEAT_MAX 7

static const unsigned char icons[] PROGMEM =
{
  0x3e, 0x63, 0x5d, 0x5d, 0x63, 0x3e, 0x00,  // 0
  0x3e, 0x7f, 0x7b, 0x41, 0x7f, 0x3e, 0x00,  // 1
  0x3e, 0x5b, 0x4d, 0x55, 0x5b, 0x3e, 0x00,  // 2
  0x3e, 0x6b, 0x5d, 0x55, 0x6b, 0x3e, 0x00,  // 3
  0x3e, 0x73, 0x75, 0x77, 0x41, 0x3e, 0x00   // 4
};

static const uint16_t rate_lut[] PROGMEM =
{
  11, 13, 15, 18, 21, 25, 28, 32, 36, 40, 45, 50, 55, 61, 68, 74, 82, 90, 98, 107, 117, 128, 140, 152, 165, 180, 195,
  212, 230, 249, 270, 293, 317, 343, 371, 402, 434, 470, 508, 549, 593, 640, 691, 746, 806, 870, 939, 1013, 1093, 1180,
  1273, 1373, 1481, 1597, 1722, 1857, 2003, 2160, 2328, 2510, 2707, 2918, 3145, 3391, 3655, 3940, 4246, 4577, 4933,
  5317, 5730, 6176, 6656, 7173, 7731, 8331, 8978, 9675, 10426, 11236, 12108, 13047, 14060, 15150, 16326, 17592, 18956,
  20427, 22011, 23717, 25556, 27538, 29673, 31973, 34452, 37122, 40000
};

void gate_isr();
void retrigger_isr();

void loop_scroll_callback(int delta);
void repeat_scroll_callback(int delta);
void points_scroll_callback(uint8_t selected, int delta);
void points_invert_callback(uint8_t p);

class DU_VSEG_PointWidget : public DUINO_Widget
{
public:
  DU_VSEG_PointWidget(uint8_t p, uint8_t parameter)
    : p_(p)
    , x_(parameter ? 89 : 64)
    , w_(parameter ? 29 : 11)
    , inverted_(false) { }

  virtual void invert(bool update_display = true)
  {
    // invert underline
    Display.draw_hline(x_, 8, w_, DUINO_SH1106::Inverse);

    // blank area
    Display.fill_rect(45, 0, 73, 7, DUINO_SH1106::Black);

    if (!inverted_)
    {
      // display point number
      Display.draw_char(45, 0, '0' + p_, DUINO_SH1106::White);

      if(invert_callback_)
      {
        invert_callback_(p_);
      }
    }

    inverted_ = !inverted_;

    if (update_display)
    {
      Display.display(45, 117, 0, 1);
    }
  }

  virtual bool inverted() const { return inverted_; }

  void attach_invert_callback(void (*callback)(uint8_t))
  {
    invert_callback_ = callback;
  }

protected:
  void (*invert_callback_)(uint8_t);

  const uint8_t p_, x_, w_;
  bool inverted_;
};

class DU_VSEG_Function : public DUINO_Function
{
public:
  DU_VSEG_Function() : DUINO_Function(0b10111100) { }

  virtual void function_setup()
  {
    // initialize values
    gate_ = retrigger_ = false;
    gate_time_ = release_time_ = 0;
    cv_current_ = cv_released_ = 0.0;
    update_cached_ = false;

    // build widget hierarchy
    widget_save_ = new DUINO_SaveWidget<ParameterValues>(121, 0);
    widget_loop_ = new DUINO_DisplayWidget(0, 12, 96, 7, DUINO_Widget::Corners);
    widget_loop_->attach_scroll_callback(loop_scroll_callback);
    widget_repeat_ = new DUINO_DisplayWidget(102, 11, 13, 9, DUINO_Widget::Full);
    widget_repeat_->attach_scroll_callback(repeat_scroll_callback);
    container_loop_repeat_ = new DUINO_WidgetContainer<2>(DUINO_Widget::Click);
    container_loop_repeat_->attach_child(widget_loop_, 0);
    container_loop_repeat_->attach_child(widget_repeat_, 1);
    container_points_ = new DUINO_WidgetContainer<8>(DUINO_Widget::Click);
    for(uint8_t i  = 0; i < 8; ++i)
    {
      widgets_points_[i] = new DU_VSEG_PointWidget((i + 1) / 2, i & 1);
      widgets_points_[i]->attach_invert_callback(points_invert_callback);
      container_points_->attach_child(widgets_points_[i], i);
      container_points_->attach_scroll_callback_array(points_scroll_callback);
    }
    container_outer_ = new DUINO_WidgetContainer<3>(DUINO_Widget::DoubleClick, 1);
    container_outer_->attach_child(widget_save_, 0);
    container_outer_->attach_child(container_loop_repeat_, 1);
    container_outer_->attach_child(container_points_, 2);

    // indicators
    indicator_gate_ = new DUINO_JackIndicator(121, 12);

    // load parameters
    widget_save_->load_params();
    for (uint8_t p = 0; p < 4; ++p)
    {
      widget_save_->params.vals.level[p] = clamp<int8_t>(widget_save_->params.vals.level[p], 0, LEVEL_MAX);
      widget_save_->params.vals.rate[p] = clamp<int8_t>(widget_save_->params.vals.rate[p], 0, RATE_MAX);
    }
    widget_save_->params.vals.loop = clamp<int8_t>(widget_save_->params.vals.loop, LOOP_MIN, LOOP_MAX);
    widget_save_->params.vals.repeat = clamp<int8_t>(widget_save_->params.vals.repeat, REPEAT_MIN, REPEAT_MAX);
    cached_params_ = widget_save_->params.vals;

    // draw title
    Display.draw_du_logo_sm(0, 0, DUINO_SH1106::White);
    Display.draw_text(16, 0, "VSEG", DUINO_SH1106::White);

    // draw save box
    Display.fill_rect(widget_save_->x() + 1, widget_save_->y() + 1, 5, 5, DUINO_SH1106::White);

    // draw fixed elements
    Display.draw_char(widget_repeat_->x() + 1, widget_repeat_->y() + 1, 'x', DUINO_SH1106::White);

    // draw parameters
    display_loop(false);
    display_repeat(false);
    display_envelope(false);

    // initialize widgets
    widget_setup(container_outer_);

    // output full display
    Display.display();

    // initialize filter
    env_lpf_ = new DUINO_Filter(DUINO_Filter::LowPass, 100.0, 0.0);

    // attach gate interrupt
    gt_attach_interrupt(GT3, gate_isr, CHANGE);
    gt_attach_interrupt(GT4, retrigger_isr, FALLING);
  }

  virtual void function_loop()
  {
    if (retrigger_)
    {
      gate_time_ = 0;
      release_time_ = 0;
      retrigger_ = false;

      if (update_cached_)
      {
        cached_params_ = widget_save_->params.vals;
        update_cached_ = false;
      }
    }

    if (gate_time_)
    {
      if (gate_)
      {
        const unsigned long elapsed = millis() - gate_time_;

        bool loop = cached_params_.loop > -1;
        const int8_t loop_start = cached_params_.loop % 3;
        bool loop_reverse = cached_params_.loop > 2;
        unsigned long pointer = rate_to_ms(1, true);
        uint8_t p = 0;
        bool reverse = false;
        int8_t repeat_count = 0;

        while (pointer < elapsed)
        {
          if (reverse)
          {
            p--;
          }
          else
          {
            p++;
          }

          // handle point 3
          if (p == 3)
          {
            // if we've hit point 3 the repeat count times...
            if (cached_params_.repeat && ++repeat_count == cached_params_.repeat)
            {
              // stop looping
              loop_reverse = loop = false;
            }

            if (loop_reverse)
            {
              reverse = true;
            }
            else
            {
              if (loop)
              {
                p = loop_start;
              }
              else
              {
                break;
              }
            }
          }

          // clear reverse flag at start point
          if (p == loop_start && reverse)
          {
            reverse = false;
          }

          // move the pointer
          if (reverse)
          {
            // if we're pointed backward, add the current point's rate
            pointer += rate_to_ms(p, true);
          }
          else
          {
            // if we're pointed forward, add the next point's rate
            pointer += rate_to_ms(p + 1, true);
          }
        }

        if (pointer < elapsed)
        {
          // done looping, sustain at point 3
          cv_current_ = level_to_cv(3, true);
        }
        else
        {
          // linearly interpolate between current and next point
          const float cv_start = level_to_cv(p, true);
          const float cv_end = level_to_cv(reverse ? p - 1 : p + 1, true);
          const float remaining = (float)(pointer - elapsed) / (float)rate_to_ms(reverse ? p : p + 1, true);
          cv_current_ = cv_end + (cv_start - cv_end) * remaining;
        }
      }
      else
      {
        if (release_time_)
        {
          const uint16_t elapsed = millis() - release_time_;
          if(elapsed < rate_to_ms(4, true) * ENV_RELEASE_HOLD)
          {
            // release
            cv_current_ = exp(ENV_RELEASE_COEFF * ((float)elapsed / (float)rate_to_ms(4, true))) * cv_released_;
          }
          else
          {
            cv_current_ = 0.0;
            release_time_ = 0;
            gate_time_ = 0;
          }
        }
        else
        {
          release_time_ = millis();
          cv_released_ = cv_current_;
        }
      }

      const float filtered_cv = env_lpf_->filter(cv_current_);
      cv_out(CO1, filtered_cv);
      cv_out(CO3, filtered_cv / 2.0);
    }
    else if (gate_)
    {
      gate_time_ = millis();
    }

    widget_loop();

    // display gate
    if (gate_ != indicator_gate_->state())
    {
      indicator_gate_->set(gate_);
      indicator_gate_->display();
    }
  }

  void gate_callback()
  {
    gate_ = gt_read_debounce(GT3);
    if (gate_)
    {
      retrigger_ = true;
    }
  }

  void retrigger_callback()
  {
    retrigger_ = true;
  }

  void widget_loop_scroll_callback(int delta)
  {
    if (adjust<int8_t>(widget_save_->params.vals.loop, delta, LOOP_MIN, LOOP_MAX))
    {
      widget_save_->mark_changed();
      update_cached_ = true;
      widget_save_->display();
      display_loop();
    }
  }

  void widget_repeat_scroll_callback(int delta)
  {
    if (adjust<int8_t>(widget_save_->params.vals.repeat, delta, REPEAT_MIN, REPEAT_MAX))
    {
      widget_save_->mark_changed();
      update_cached_ = true;
      widget_save_->display();
      display_repeat();
    }
  }

  void widgets_points_scroll_callback(uint8_t selected, int delta)
  {
    const uint8_t p = (selected + 1) / 2;
    const uint8_t parameter = selected & 1;

    if (parameter)
    {
      // adjust rate
      if(adjust<int8_t>(widget_save_->params.vals.rate[p - 1], delta, 0, RATE_MAX))
      {
        widget_save_->mark_changed();
        update_cached_ = true;
        widget_save_->display();
        display_plr(p);
        display_loop();
        display_envelope();
      }
    }
    else
    {
      // adjust level
      if(adjust<int8_t>(widget_save_->params.vals.level[p], delta, 0, LEVEL_MAX))
      {
        widget_save_->mark_changed();
        update_cached_ = true;
        widget_save_->display();
        display_plr(p);
        display_envelope();
      } 
    }
  }

  void widgets_points_invert_callback(uint8_t p)
  {
    display_plr(p);
  }

private:
  float level_to_cv(uint8_t p, bool use_cached = false)
  {
    if (p == 4)
    {
      return 0.0;
    }

    const int8_t level = (use_cached ? cached_params_ : widget_save_->params.vals).level[p];
    return (float)level / 9.9;
  }

  uint16_t rate_to_ms(uint8_t p, bool use_cached = false)
  {
    if (p == 0)
    {
      return 0;
    }

    const int8_t rate = (use_cached ? cached_params_ : widget_save_->params.vals).rate[p - 1];
    return (uint16_t)pgm_read_word(&rate_lut[rate]);
  }

  uint8_t level_to_y(uint8_t p)
  {
    if (p == 4)
    {
      return 57;
    }

    return 24 + (99 - widget_save_->params.vals.level[p]) / 3;
  }

  uint8_t rate_to_x(uint8_t p)
  {
    if (p == 0)
    {
      return 0;
    }

    return rate_to_x(p - 1) + 6 + (widget_save_->params.vals.rate[p - 1] / 4) + (p == 4 ? 2 : 0);
  }

  void display_plr(uint8_t p, bool update = true)
  {
    // blank area
    Display.fill_rect(64, 0, 54, 7, DUINO_SH1106::Black);

    // draw fixed elements
    Display.draw_char(54, 0, 'L', DUINO_SH1106::White);
    Display.draw_pixel(61, 1, DUINO_SH1106::White);
    Display.draw_pixel(61, 5, DUINO_SH1106::White);
    Display.draw_char(79, 0, 'R', DUINO_SH1106::White);
    Display.draw_pixel(86, 1, DUINO_SH1106::White);
    Display.draw_pixel(86, 5, DUINO_SH1106::White);

    uint8_t i;
    
    // display level
    int8_t level = widget_save_->params.vals.level[p];
    if (level)
    {
      i = 0;
      while (level)
      {
        Display.draw_char(70 - 6 * i, 0, '0' + level % 10, DUINO_SH1106::White);
        level /= 10;
        i++;
      }
    }
    else
    {
      Display.draw_char(70, 0, '0', DUINO_SH1106::White);
    }

    // display rate
    uint16_t rate = rate_to_ms(p);
    if (rate)
    {
      i = 0;
      while (rate)
      {
        Display.draw_char(113 - 6 * i, 0, '0' + rate % 10, DUINO_SH1106::White);
        rate /= 10;
        i++;
      }
    }
    else
    {
      Display.draw_char(113, 0, '0', DUINO_SH1106::White);
    }

    if (update)
    {
      Display.display(45, 117, 0, 0);
    }
  }

  void display_loop(bool update = true)
  {
    // blank area
    Display.fill_rect(widget_loop_->x() + 2, widget_loop_->y() + 1, widget_loop_->width() - 4,
        widget_loop_->height() - 2, DUINO_SH1106::Black);

    if (widget_save_->params.vals.loop > -1)
    {
      const uint8_t x1 = rate_to_x(widget_save_->params.vals.loop % 3) + 2;
      const uint8_t x2 = rate_to_x(3) + 3;
      const uint8_t w = x2 - x1 + 1;

      // draw shaft
      Display.draw_hline(x1, widget_loop_->y() + 3, w, DUINO_SH1106::White);

      // draw forward arrow head
      Display.draw_vline(x2 - 1, widget_loop_->y() + 2, 3, DUINO_SH1106::White);
      Display.draw_vline(x2 - 2, widget_loop_->y() + 1, 5, DUINO_SH1106::White);
      
      // draw reverse arrow head
      if (widget_save_->params.vals.loop > 2)
      {
        Display.draw_vline(x1 + 1, widget_loop_->y() + 2, 3, DUINO_SH1106::White);
        Display.draw_vline(x1 + 2, widget_loop_->y() + 1, 5, DUINO_SH1106::White);
      }
    }

    if (update)
    {
      widget_loop_->display();
    }
  }

  void display_repeat(bool update = true)
  {
    Display.fill_rect(widget_repeat_->x() + 7, widget_repeat_->y() + 1, 5, 7,
        widget_repeat_->inverted() ? DUINO_SH1106::White : DUINO_SH1106::Black);

    const unsigned char c = widget_save_->params.vals.repeat ? '0' + widget_save_->params.vals.repeat : 'C';
    Display.draw_char(widget_repeat_->x() + 7, widget_repeat_->y() + 1, c,
        widget_repeat_->inverted() ? DUINO_SH1106::Black : DUINO_SH1106::White);

    if (update)
    {
      widget_repeat_->display();
    }
  }

  void display_envelope(bool update = true)
  {
    Display.fill_rect(0, 24, 128, 40, DUINO_SH1106::Black);

    for (uint8_t p = 0; p < 5; ++p)
    {
      const uint8_t x = rate_to_x(p);
      const uint8_t y = level_to_y(p);

      Display.draw_bitmap_7(x, y, icons, p, DUINO_SH1106::White);

      if (p < 3)
      {
        Display.draw_line(x + 5, y + 3, rate_to_x(p + 1), level_to_y(p + 1) + 3, DUINO_SH1106::White);
      }
      else if (p == 3)
      {
        Display.draw_hline(x + 6, y + 3, 2, DUINO_SH1106::White);

        // draw exponential decay
        uint8_t xs = x + 8;
        uint8_t ys = y + 3;
        const uint8_t x4 = rate_to_x(4);
        const uint8_t yh = 57 - y;
        const uint8_t xw = x4 - xs;
        if (xs == x4)
        {
          Display.draw_vline(xs, ys, yh + 1, DUINO_SH1106::White);
        }
        else
        {
          while (xs < x4)
          {
            uint8_t ye = 60 - (uint8_t)((float)yh * exp(-(float)((10 + (128 / xw)) * (xs - x - 7)) / (float)(x4)));
            Display.draw_line(xs, ys, xs + 1, ye, DUINO_SH1106::White);
            xs++;
            ys = ye;
          }
        }
      }
    }

    if (update)
    {
      Display.display(0, 127, 3, 7);
    }
  }

  struct ParameterValues
  {
    int8_t level[4];  // levels of points 0 - 3 (0 - 99)
    int8_t rate[4];   // rates of points 1 - 4 (0 - 96)
    int8_t loop;      // -1 = off, loop % 3 = loop start, loop > 2 = loop back and forth
    int8_t repeat;    // 0 = continuous, 1 - 7 = finite repeats
  };

  ParameterValues cached_params_;

  DUINO_WidgetContainer<3> * container_outer_;
  DUINO_WidgetContainer<2> * container_loop_repeat_;
  DUINO_WidgetContainer<8> * container_points_;
  DUINO_SaveWidget<ParameterValues> * widget_save_;
  DUINO_DisplayWidget * widget_loop_;
  DUINO_DisplayWidget * widget_repeat_;
  DU_VSEG_PointWidget * widgets_points_[8];

  DUINO_JackIndicator * indicator_gate_;

  DUINO_Filter * env_lpf_;

  volatile bool gate_, retrigger_;
  unsigned long gate_time_, release_time_;
  float cv_current_, cv_released_;
  bool update_cached_;
};

DU_VSEG_Function * function;

void gate_isr() { function->gate_callback(); }
void retrigger_isr() { function->retrigger_callback(); }

void loop_scroll_callback(int delta) { function->widget_loop_scroll_callback(delta); }
void repeat_scroll_callback(int delta) { function->widget_repeat_scroll_callback(delta); }
void points_scroll_callback(uint8_t selected, int delta) { function->widgets_points_scroll_callback(selected, delta); }
void points_invert_callback(uint8_t p) { function->widgets_points_invert_callback(p); }

void setup()
{
  function = new DU_VSEG_Function();

  function->begin();
}

void loop()
{
  function->function_loop();
}
