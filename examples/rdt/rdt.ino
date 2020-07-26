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
 * DU-INO DU-RDT Emulator
 * Aaron Mavrinac <aaron@logick.ca>
 *
 * JACK    FUNCTION
 * ----    --------
 * GT1 O - upper LFSR out
 * GT2 O - lower LFSR out
 * GT3 I - clock in
 * GT4 I - 1s in
 * CI1   - pattern loop (use knob to toggle)
 * CI2   - D in (set knob to -10V)
 * CI3   - T in (set knob to -10V)
 * CI4   - 
 * OFFST -
 * CO1   - clock out
 * CO2   - D out
 * CO3   - D inverted out
 * CO4   - T out
 * FNCTN -
 *
 * SWITCH CONFIGURATION
 * --------------------
 * SG2    [_][_]    SG1
 * SG4    [^][^]    SG3
 * SC2    [_][_]    SC1
 * SC4    [_][_]    SC3
 */

#include <du-ino_function.h>
#include <du-ino_widgets.h>
#include <du-ino_indicators.h>
#include <du-ino_save.h>
#include <du-ino_clock.h>
#include <du-ino_utils.h>
#include <avr/pgmspace.h>

#define CLOCK_BPM_MAX 300
#define SWING_MIN 0
#define SWING_MAX 6
#define LFSR_MIN 1
#define LFSR_MAX 8

static const unsigned char loop_icons[] PROGMEM =
{
  0x30, 0x48, 0x84, 0x84, 0x80, 0x84, 0x8e, 0x9f,  // left part
  0x84, 0x84, 0x84, 0x84, 0x84, 0x84, 0x48, 0x30   // right part
};

void clock_ext_isr();

void clock_callback();
void external_callback();

void clock_scroll_callback(int delta);
void swing_scroll_callback(int delta);
void lfsr_scroll_callback(uint8_t half, int delta);

class DU_RDT_Function : public DUINO_Function
{
public:
  DU_RDT_Function() : DUINO_Function(0b00001100) { }

  virtual void function_setup()
  {
    // build widget hierarchy
    container_outer_ = new DUINO_WidgetContainer<3>(DUINO_Widget::DoubleClick, 2);
    widget_save_ = new DUINO_SaveWidget<ParameterValues>(121, 0);
    container_outer_->attach_child(widget_save_, 0);
    container_top_ = new DUINO_WidgetContainer<2>(DUINO_Widget::Click);
    widget_clock_ = new DUINO_DisplayWidget(73, 0, 19, 9, DUINO_Widget::Full);
    widget_clock_->attach_scroll_callback(clock_scroll_callback);
    container_top_->attach_child(widget_clock_, 0);
    widget_swing_ = new DUINO_DisplayWidget(97, 0, 19, 9, DUINO_Widget::Full);
    widget_swing_->attach_scroll_callback(swing_scroll_callback);
    container_top_->attach_child(widget_swing_, 1);
    container_outer_->attach_child(container_top_, 1);
    widgets_lfsr_ = new DUINO_MultiDisplayWidget<2>(5, 19, 13, 13, 105, false, DUINO_Widget::Corners,
        DUINO_Widget::Click);
    widgets_lfsr_->attach_scroll_callback_array(lfsr_scroll_callback);
    container_outer_->attach_child(widgets_lfsr_, 2);

    // indicators
    for(uint8_t i = 0; i < 2; ++i)
    {
      indicator_lfsr_[i] = new DUINO_JackIndicator(widgets_lfsr_->x(i) + 3, widgets_lfsr_->y(i) + 20);
    }
    indicator_1s_in_ = new DUINO_JackIndicator(23, 10);
    indicator_d_in_ = new DUINO_JackIndicator(32, 39);
    indicator_t_in_ = new DUINO_JackIndicator(32, 48);
    indicator_d_out_ = new DUINO_JackIndicator(89, 39);
    indicator_dinv_out_ = new DUINO_JackIndicator(89, 48);
    indicator_t_out_ = new DUINO_JackIndicator(89, 57);

    // initialize interface
    lfsr_loop_ = jack_1s_ = d_out_ = t_out_ = false;
    jack_d_ = jack_t_ = -1;
    update_lfsr_jacks = update_pattern_dt_jacks = false;

    Clock.begin();
    Clock.attach_clock_callback(clock_callback);
    Clock.attach_external_callback(external_callback);

    gt_attach_interrupt(GT3, clock_ext_isr, CHANGE);

    // draw top line
    Display.draw_du_logo_sm(0, 0, DUINO_SH1106::White);
    Display.draw_text(16, 0, "RDT", DUINO_SH1106::White);

    // draw save box
    Display.fill_rect(widget_save_->x() + 1, widget_save_->y() + 1, 5, 5, DUINO_SH1106::White);

    // draw big circles
    // FIXME: have to do this manually because draw_circle doesn't quite get it right
    for (uint8_t i = 0; i < 2; ++i)
    {
      for (uint8_t j = 0; j < 2; ++j)
      {
        Display.draw_vline(j * 22 + i * 105, 22, 7, DUINO_SH1106::White);
        Display.draw_hline(8 + i * 105, 14 + j * 22, 7, DUINO_SH1106::White);
        for (uint8_t k = 0; k < 2; ++k)
        {
          Display.draw_vline(1 + k * 20 + i * 105, 20 + j * 9, 2, DUINO_SH1106::White);
          Display.draw_hline(6 + k * 9 + i * 105, 15 + j * 20, 2, DUINO_SH1106::White);
          Display.draw_pixel(2 + k * 18 + i * 105, 19 + j * 12, DUINO_SH1106::White);
          Display.draw_pixel(3 + k * 16 + i * 105, 18 + j * 14, DUINO_SH1106::White);
          Display.draw_pixel(3 + k * 16 + i * 105, 17 + j * 16, DUINO_SH1106::White);
          Display.draw_pixel(4 + k * 14 + i * 105, 17 + j * 16, DUINO_SH1106::White);
          Display.draw_pixel(5 + k * 12 + i * 105, 16 + j * 18, DUINO_SH1106::White);
        }
      }
    }

    // draw 1s down arrows
    for (uint8_t i = 0; i < 2; ++i)
    {
      Display.draw_pixel(25, 18 + i * 3, DUINO_SH1106::White);
      Display.draw_pixel(26, 19 + i * 3, DUINO_SH1106::White);
      Display.draw_pixel(27, 18 + i * 3, DUINO_SH1106::White);
    }

    // draw D and T lines
    for (uint8_t i = 0; i < 2; ++i)
    {
      for (uint8_t x = 0; x < 20; ++x)
      {
        for (uint8_t y = 0; y < 3; ++y)
        {
          if ((x - y) & 2)
          {
            Display.draw_pixel(40 + x, 41 + (i * 9) + y, DUINO_SH1106::White);
          }
        }
      }
    }
    Display.draw_char(62, 39, 'D', DUINO_SH1106::White);
    Display.draw_char(62, 48, 'T', DUINO_SH1106::White);
    Display.fill_rect(69, 41, 18, 3, DUINO_SH1106::White);
    Display.draw_pixel(87, 42, DUINO_SH1106::White);
    Display.fill_rect(69, 50, 11, 3, DUINO_SH1106::White);
    Display.draw_pixel(79, 53, DUINO_SH1106::White);
    for (uint8_t i = 0; i < 6; ++i)
    {
      Display.draw_vline(80 + i, 51 + i, 4, DUINO_SH1106::White);
    }
    Display.draw_vline(86, 57, 3, DUINO_SH1106::White);
    Display.draw_vline(87, 58, 2, DUINO_SH1106::White);

    // load settings
    widget_save_->load_params();

    widget_save_->params.vals.clock_bpm = clamp<int16_t>(widget_save_->params.vals.clock_bpm, 0, CLOCK_BPM_MAX);
    Clock.set_bpm(widget_save_->params.vals.clock_bpm);

    widget_save_->params.vals.swing = clamp<int8_t>(widget_save_->params.vals.swing, SWING_MIN, SWING_MAX);
    Clock.set_swing(widget_save_->params.vals.swing);

    for (uint8_t i = 0; i < 2; ++i)
    {
      widget_save_->params.vals.lfsr[i] = clamp<int8_t>(widget_save_->params.vals.lfsr[i], LFSR_MIN, LFSR_MAX);
    }

    // draw parameters
    display_clock(widget_clock_->x() + 1, widget_clock_->y() + 1, widget_save_->params.vals.clock_bpm,
        DUINO_SH1106::White);
    display_swing(widget_swing_->x() + 1, widget_swing_->y() + 1, widget_save_->params.vals.swing, DUINO_SH1106::White);
    Display.draw_char(110, 1, '%', DUINO_SH1106::White);
    for (uint8_t i = 0; i < 2; ++i)
    {
      Display.draw_char(widgets_lfsr_->x(i) + 4, widgets_lfsr_->y(i) + 3, '0' + widget_save_->params.vals.lfsr[i],
          DUINO_SH1106::White);
      draw_lfsr_arrow(24 + (i * 40) + ((widget_save_->params.vals.lfsr[i] - 1) * 5), 29, DUINO_SH1106::White);
    }

    widget_setup(container_outer_);
    Display.display();
  }

  virtual void function_loop()
  {
    widget_loop();

    // store current loop state
    const bool lfsr_loop_last = lfsr_loop_;

    // update jack states
    lfsr_loop_ = gt_read(CI1);
    jack_1s_ = gt_read(GT4);
    const float d_val = cv_read(CI2);
    if (d_val < -5.0)
    {
      jack_d_ = -1;
    }
    else
    {
      jack_d_ = d_val > 3.0;  // FIXME: move DIGITAL_THRESH constant to function header?
    }
    const float t_val = cv_read(CI3);
    if (t_val < -5.0)
    {
      jack_t_ = -1;
    }
    else
    {
      jack_t_ = t_val > 3.0;  // FIXME: move DIGITAL_THRESH constant to function header?
    }


    // display jack states
    if (lfsr_loop_ != lfsr_loop_last)
    {
      display_lfsr_loop(56, 13, lfsr_loop_, DUINO_SH1106::White);
      Display.display(56, 71, 1, 2);
    }

    if (jack_1s_ != indicator_1s_in_->state())
    {
      indicator_1s_in_->set(jack_1s_);
      indicator_1s_in_->display();
    }

    if (jack_d_ != indicator_d_in_->state())
    {
      indicator_d_in_->set(jack_d_);
      indicator_d_in_->display();
    }

    if (jack_t_ != indicator_t_in_->state())
    {
      indicator_t_in_->set(jack_t_);
      indicator_t_in_->display();
    }

    if (update_lfsr_jacks)
    {
      indicator_lfsr_[0]->display();
      indicator_lfsr_[1]->display();
      update_lfsr_jacks = false;
    }

    if (update_pattern_dt_jacks)
    {
      Display.display(25, 102, 3, 3);
      indicator_d_out_->display();
      indicator_dinv_out_->display();
      indicator_t_out_->display();
      update_pattern_dt_jacks = false;
    }
  }

  void clock_clock_callback()
  {
    // output clock
    gt_out(CO1, Clock.state());

    if (Clock.state())
    {
      // LFSR
      if (lfsr_loop_)
      {
        widget_save_->params.vals.pattern =
            (widget_save_->params.vals.pattern << 1) | (widget_save_->params.vals.pattern >> 15);
      }
      else
      {
        const uint16_t lfsr_bit = jack_1s_ ? 1 :
            ((widget_save_->params.vals.pattern >> 15) ^ ((widget_save_->params.vals.pattern >> 13) & 1)) ^
            ((widget_save_->params.vals.pattern >> 12) & 1) ^ ((widget_save_->params.vals.pattern >> 10) & 1);
        widget_save_->params.vals.pattern = (widget_save_->params.vals.pattern << 1) | lfsr_bit;
      }

      uint8_t jacks = 0;
      if (widget_save_->params.vals.pattern & (1 << ((widget_save_->params.vals.lfsr[0] - 1))))
      {
        jacks |= (1 << GT1);
      }
      if (widget_save_->params.vals.pattern & (1 << ((widget_save_->params.vals.lfsr[1] + 7))))
      {
        jacks |= (1 << GT2);
      }
      
      // D & T
      if (jack_t_ < 0)
      {
        t_out_ = d_out_ ? t_out_ : !t_out_;
      }
      else
      {
        t_out_ = jack_t_ ? !t_out_ : t_out_;
      }

      if (jack_d_ < 0)
      {
        d_out_ = !d_out_;
      }
      else
      {
        d_out_ = jack_d_;
      }

      // output all jacks
      cv_hold(true);
      gt_out(CO2, d_out_);
      gt_out(CO3, !d_out_);
      gt_out(CO4, t_out_);
      gt_out_multi(jacks, true);
      cv_hold(false);

      // display pattern
      display_pattern(25, 24, widget_save_->params.vals.pattern, DUINO_SH1106::White);
      
      // display jacks
      indicator_lfsr_[0]->set(jacks & (1 << GT1));
      indicator_lfsr_[1]->set(jacks & (1 << GT2));
      indicator_d_out_->set(d_out_);
      indicator_dinv_out_->set(!d_out_);
      indicator_t_out_->set(t_out_);

      update_pattern_dt_jacks = true;   
    }
    else
    {
      gt_out_multi((1 << GT1) | (1 << GT2), false);

      indicator_lfsr_[0]->set(false);
      indicator_lfsr_[1]->set(false);
    }

    update_lfsr_jacks = true;
  }

  void clock_external_callback()
  {
    widget_save_->params.vals.clock_bpm = 0;
    
    Display.fill_rect(widget_clock_->x() + 1, widget_clock_->y() + 1, 17, 7,
        widget_clock_->inverted() ? DUINO_SH1106::White : DUINO_SH1106::Black);
    display_clock(widget_clock_->x() + 1, widget_clock_->y() + 1, widget_save_->params.vals.clock_bpm,
        widget_clock_->inverted() ? DUINO_SH1106::Black : DUINO_SH1106::White);
    widget_clock_->display();
  }

  void widget_clock_scroll_callback(int delta)
  {
    if (adjust<int16_t>(widget_save_->params.vals.clock_bpm, delta, 0, CLOCK_BPM_MAX))
    {
      Clock.set_bpm(widget_save_->params.vals.clock_bpm);
      widget_save_->mark_changed();
      widget_save_->display();
      Display.fill_rect(widget_clock_->x() + 1, widget_clock_->y() + 1, 17, 7, DUINO_SH1106::White);
      display_clock(widget_clock_->x() + 1, widget_clock_->y() + 1, widget_save_->params.vals.clock_bpm,
          DUINO_SH1106::Black);
      widget_clock_->display();
    }
  }

  void widget_swing_scroll_callback(int delta)
  {
    if (adjust<int8_t>(widget_save_->params.vals.swing, delta, SWING_MIN, SWING_MAX))
    {
      Clock.set_swing(widget_save_->params.vals.swing);
      widget_save_->mark_changed();
      widget_save_->display();
      Display.fill_rect(widget_swing_->x() + 1, widget_swing_->y() + 1, 11, 7, DUINO_SH1106::White);
      display_swing(widget_swing_->x() + 1, widget_swing_->y() + 1, widget_save_->params.vals.swing, DUINO_SH1106::Black);
      widget_swing_->display();
    }
  }

  void widgets_lfsr_scroll_callback(uint8_t half, int delta)
  {
    if (adjust<int8_t>(widget_save_->params.vals.lfsr[half], delta, LFSR_MIN, LFSR_MAX))
    {
      widget_save_->mark_changed();
      widget_save_->display();
      Display.fill_rect(24 + (half * 40) + ((widget_save_->params.vals.lfsr[half] - 1) * 5), 29, 5, 3,
          DUINO_SH1106::Black);
      Display.fill_rect(widgets_lfsr_->x(half) + 4, widgets_lfsr_->y(half) + 3, 5, 7, DUINO_SH1106::Black);
      Display.draw_char(widgets_lfsr_->x(half) + 4, widgets_lfsr_->y(half) + 3,
          '0' + widget_save_->params.vals.lfsr[half], DUINO_SH1106::White);
      draw_lfsr_arrow(24 + (half * 40) + ((widget_save_->params.vals.lfsr[half] - 1) * 5), 29, DUINO_SH1106::White);
      widgets_lfsr_->display();
      Display.display(23, 103, 3, 3);
    }
  }

private:
  void display_clock(int16_t x, int16_t y, uint16_t bpm, DUINO_SH1106::Color color)
  {
    if (bpm == 0)
    {
      Display.draw_text(x, y, "EXT", color);
    }
    else
    {
      Display.draw_char(x, y, '0' + bpm / 100, color);
      Display.draw_char(x + 6, y, '0' + (bpm % 100) / 10, color);
      Display.draw_char(x + 12, y, '0' + bpm % 10, color);
    }
  }

  void display_swing(int16_t x, int16_t y, uint8_t swing, DUINO_SH1106::Color color)
  {
    const uint8_t swing_percent = 50 + 4 * swing;
    Display.draw_char(x, y, '0' + swing_percent / 10, color);
    Display.draw_char(x + 6, y, '0' + swing_percent % 10, color);
  }

  void display_pattern(int16_t x, int16_t y, uint16_t pattern, DUINO_SH1106::Color color)
  {
    for (uint8_t i = 0; i < 16; ++i)
    {
      Display.fill_rect(25 + i * 5, 24, 3, 3, pattern & (1 << i) ? DUINO_SH1106::White : DUINO_SH1106::Black);
    }
  }

  void display_lfsr_loop(int16_t x, int16_t y, bool lfsr_loop, DUINO_SH1106::Color color)
  {
    if (lfsr_loop)
    {
      for (uint8_t i = 0; i < 2; ++i)
      {
        Display.draw_bitmap_8(56 + i * 8, 13, loop_icons, i, DUINO_SH1106::White);
      }
    }
    else
    {
      Display.fill_rect(56, 13, 16, 8, DUINO_SH1106::Black);
    }
  }

  void draw_lfsr_arrow(int16_t x, int16_t y, DUINO_SH1106::Color color)
  {
    Display.draw_pixel(x, y + 2, color);
    Display.draw_vline(x + 1, y + 1, 2, color);
    Display.draw_vline(x + 2, y, 3, color);
    Display.draw_vline(x + 3, y + 1, 2, color);
    Display.draw_pixel(x + 4, y + 2, color);
  }

  struct ParameterValues
  {
    uint16_t pattern;
    int16_t clock_bpm;
    int8_t swing;
    int8_t lfsr[2];
  };

  DUINO_WidgetContainer<3> * container_outer_;
  DUINO_WidgetContainer<2> * container_top_;
  DUINO_SaveWidget<ParameterValues> * widget_save_;
  DUINO_DisplayWidget * widget_clock_;
  DUINO_DisplayWidget * widget_swing_;
  DUINO_MultiDisplayWidget<2> * widgets_lfsr_;

  DUINO_JackIndicator * indicator_lfsr_[2];
  DUINO_JackIndicator * indicator_1s_in_, * indicator_d_in_, * indicator_t_in_, * indicator_d_out_,
                      * indicator_dinv_out_, * indicator_t_out_;

  bool lfsr_loop_, jack_1s_, d_out_, t_out_;
  int8_t jack_d_, jack_t_;
  volatile bool update_lfsr_jacks, update_pattern_dt_jacks;
};

DU_RDT_Function * function;

void clock_ext_isr()
{
  Clock.on_jack(function->gt_read_debounce(DUINO_Function::GT3));
}

void clock_callback() { function->clock_clock_callback(); }
void external_callback() { function->clock_external_callback(); }

void clock_scroll_callback(int delta) { function->widget_clock_scroll_callback(delta); }
void swing_scroll_callback(int delta) { function->widget_swing_scroll_callback(delta); }
void lfsr_scroll_callback(uint8_t half, int delta) { function->widgets_lfsr_scroll_callback(half, delta); }

void setup()
{
  function = new DU_RDT_Function();

  function->begin();
}

void loop()
{
  function->function_loop();
}
