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
 * DU-INO PLSR Stochastic Drum Sequencer Function
 * Aaron Mavrinac <aaron@logick.ca>
 *
 * JACK    FUNCTION
 * ----    --------
 * GT1 O -
 * GT2 O - clock out
 * GT3 I - clock in
 * GT4 I - reset in
 * CI1   - hit probability 1 (0V - 5V)
 * CI2   - hit probability 2 (0V - 5V)
 * CI3   - hit probability 3 (0V - 5V)
 * CI4   - hit probability 4 (0V - 5V)
 * OFFST -
 * CO1   - pattern 1 trigger out
 * CO2   - pattern 2 trigger out
 * CO3   - pattern 3 trigger out
 * CO4   - pattern 4 trigger out
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
#include <du-ino_save.h>
#include <du-ino_clock.h>
#include <du-ino_utils.h>

#define CLOCK_BPM_MAX 300
#define STEP_MIN 1
#define STEP_MAX 16
#define SWING_MIN 0
#define SWING_MAX 6
#define N_PATTERNS 6

static const unsigned char icons[] PROGMEM =
{
  0x3e, 0x7f, 0x7f, 0x7f, 0x7f, 0x3e, 0x00,  // full
  0x3e, 0x7f, 0x7b, 0x41, 0x7f, 0x3e, 0x00,  // 1
  0x3e, 0x5b, 0x4d, 0x55, 0x5b, 0x3e, 0x00,  // 2
  0x3e, 0x6b, 0x5d, 0x55, 0x6b, 0x3e, 0x00,  // 3
  0x3e, 0x73, 0x75, 0x77, 0x41, 0x3e, 0x00   // 4
};

static const DUINO_Function::Jack in_jacks[4] =
  {DUINO_Function::CI1, DUINO_Function::CI2, DUINO_Function::CI3, DUINO_Function::CI4};
static const DUINO_Function::Jack out_jacks[4] =
  {DUINO_Function::CO1, DUINO_Function::CO2, DUINO_Function::CO3, DUINO_Function::CO4};

void clock_ext_isr();
void reset_isr();

void clock_callback();
void external_callback();

void measures_scroll_callback(int delta);
void clock_scroll_callback(int delta);
void swing_scroll_callback(int delta);
void patterns_click_callback_0(uint8_t step);
void patterns_click_callback_1(uint8_t step);
void patterns_click_callback_2(uint8_t step);
void patterns_click_callback_3(uint8_t step);

class DU_PLSR_Function : public DUINO_Function
{
public:
  DU_PLSR_Function() : DUINO_Function(0b00001100) { }

  virtual void function_setup()
  {
    // build widget hierarchy
    container_outer_ = new DUINO_WidgetContainer<6>(DUINO_Widget::DoubleClick, 2);
    widget_save_ = new DUINO_SaveWidget<ParameterValues>(121, 0);
    container_outer_->attach_child(widget_save_, 0);
    container_top_ = new DUINO_WidgetContainer<3>(DUINO_Widget::Click);
    widget_measures_ = new DUINO_DisplayWidget(56, 0, 13, 9, DUINO_Widget::Full);
    widget_measures_->attach_scroll_callback(measures_scroll_callback);
    container_top_->attach_child(widget_measures_, 0);
    widget_clock_ = new DUINO_DisplayWidget(73, 0, 19, 9, DUINO_Widget::Full);
    widget_clock_->attach_scroll_callback(clock_scroll_callback);
    container_top_->attach_child(widget_clock_, 1);
    widget_swing_ = new DUINO_DisplayWidget(97, 0, 19, 9, DUINO_Widget::Full);
    widget_swing_->attach_scroll_callback(swing_scroll_callback);
    container_top_->attach_child(widget_swing_, 2);
    container_outer_->attach_child(container_top_, 1);
    for (uint8_t i = 0; i < 4; ++i)
    {
      widgets_patterns_[i] = new DUINO_MultiDisplayWidget<STEP_MAX>(0, 16 + 11 * i, 8, 9, 8, false,
          DUINO_Widget::Corners, DUINO_Widget::Scroll);
      container_outer_->attach_child(widgets_patterns_[i], i + 2);
    }
    widgets_patterns_[0]->attach_click_callback_array(patterns_click_callback_0);
    widgets_patterns_[1]->attach_click_callback_array(patterns_click_callback_1);
    widgets_patterns_[2]->attach_click_callback_array(patterns_click_callback_2);
    widgets_patterns_[3]->attach_click_callback_array(patterns_click_callback_3);

    Clock.begin();
    Clock.attach_clock_callback(clock_callback);
    Clock.attach_external_callback(external_callback);

    gt_attach_interrupt(GT3, clock_ext_isr, CHANGE);
    gt_attach_interrupt(GT4, reset_isr, RISING);

    randomSeed(cv_read(CI1));

    // initialize interface
    current_step_ = -1;
    displayed_step_ = -1;

    // load params
    widget_save_->load_params();

    widget_save_->params.vals.step_count = clamp<int8_t>(widget_save_->params.vals.step_count, STEP_MIN, STEP_MAX);
    
    widget_save_->params.vals.clock_bpm = clamp<int16_t>(widget_save_->params.vals.clock_bpm, 0, CLOCK_BPM_MAX);
    Clock.set_bpm(widget_save_->params.vals.clock_bpm);

    widget_save_->params.vals.swing = clamp<int8_t>(widget_save_->params.vals.swing, SWING_MIN, SWING_MAX);
    Clock.set_swing(widget_save_->params.vals.swing);

    for (uint8_t p = 0; p < 64; ++p)
    {
      if (widget_save_->params.vals.pattern[p] > N_PATTERNS - 1)
      {
        widget_save_->params.vals.pattern[p] = 0;
      }
      display_pattern_dot(p / STEP_MAX, p % STEP_MAX);
    }

    // draw top line
    Display.draw_du_logo_sm(0, 0, DUINO_SH1106::White);
    Display.draw_text(16, 0, "PLSR", DUINO_SH1106::White);

    // draw save box
    Display.fill_rect(widget_save_->x() + 1, widget_save_->y() + 1, 5, 5, DUINO_SH1106::White);

    // draw quarter measure markers
    for (uint8_t i = 0; i < 3; ++i)
    {
      Display.fill_rect(32 * i + 31, 62, 2, 2, DUINO_SH1106::White);
    }

    // draw parameters
    display_step_count(widget_measures_->x() + 1, widget_measures_->y() + 1, widget_save_->params.vals.step_count,
        DUINO_SH1106::White);
    display_clock(widget_clock_->x() + 1, widget_clock_->y() + 1, widget_save_->params.vals.clock_bpm,
        DUINO_SH1106::White);
    display_swing(widget_swing_->x() + 1, widget_swing_->y() + 1, widget_save_->params.vals.swing, DUINO_SH1106::White);
    Display.draw_char(110, 1, '%', DUINO_SH1106::White);

    widget_setup(container_outer_);
    Display.display();
  }

  virtual void function_loop()
  {
    widget_loop();
  }

  void clock_ext_callback()
  {
    Clock.on_jack(gt_read_debounce(DUINO_Function::GT3));
  }

  void reset_callback()
  {
    current_step_ = -1;
    Clock.reset();
  }

  void clock_clock_callback()
  {
    // output clock
    gt_out(GT2, Clock.state());

    if (Clock.state())
    {
      // increment step
      current_step_++;
      current_step_ %= widget_save_->params.vals.step_count;

      // check each dot for trigger probability
      uint8_t jacks = 0;
      for (uint8_t bank = 0; bank < 4; ++bank)
      {
        if (stochastic_trigger(bank, current_step_))
        {
          jacks |= (1 << out_jacks[bank]);
        }
      }

      // output triggers
      gt_out_multi(jacks, true, true);

      // display step
      invert_step(displayed_step_);
      displayed_step_ = current_step_;
      invert_step(displayed_step_);
    }
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

  void widget_measures_scroll_callback(int delta)
  {
    if (adjust<int8_t>(widget_save_->params.vals.step_count, delta, STEP_MIN, STEP_MAX))
    {
      widget_save_->mark_changed();
      widget_save_->display();
      Display.fill_rect(widget_measures_->x() + 1, widget_measures_->y() + 1, 11, 7, DUINO_SH1106::White);
      display_step_count(widget_measures_->x() + 1, widget_measures_->y() + 1, widget_save_->params.vals.step_count,
          DUINO_SH1106::Black);
      widget_measures_->display();
    }
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

  void widgets_patterns_click_callback(uint8_t bank, uint8_t step)
  {
    const uint8_t p = STEP_MAX * bank + step;
    widget_save_->params.vals.pattern[p]++;
    widget_save_->params.vals.pattern[p] %= N_PATTERNS;
    widget_save_->mark_changed();
    widget_save_->display();
    display_pattern_dot(bank, step);
    widgets_patterns_[bank]->display();
  }

private:
  bool stochastic_trigger(uint8_t bank, uint8_t step)
  {
    const uint8_t dot = widget_save_->params.vals.pattern[STEP_MAX * bank + step];
    switch (dot)
    {
      case 0:
        return false;
        break;
      case 1:
        return true;
        break;
      default:
        return random(10000) < (long)(cv_read(in_jacks[dot - 2]) * 1000.0);
        break;
    }
  }

  void display_step_count(int16_t x, int16_t y, uint16_t count, DUINO_SH1106::Color color)
  {
    if (count > 9)
    {
      Display.draw_char(x, y, '0' + count / 10, color);
    }
    Display.draw_char(x + 6, y, '0' + count % 10, color);
  }

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

  void display_pattern_dot(uint8_t bank, uint8_t step)
  {
    const uint8_t p = STEP_MAX * bank + step;
    Display.fill_rect(widgets_patterns_[bank]->x(step) + 1, widgets_patterns_[bank]->y(step) + 1, 6, 7,
        DUINO_SH1106::Black);
    if (widget_save_->params.vals.pattern[p])
    {
      Display.draw_bitmap_7(widgets_patterns_[bank]->x(step) + 1, widgets_patterns_[bank]->y(step) + 1, icons,
          widget_save_->params.vals.pattern[p] - 1, DUINO_SH1106::White);
    }
  }

  void invert_step(int8_t step)
  {
    if (step > -1)
    {
      Display.fill_rect(step * 8 + 3, 11, 2, 5, DUINO_SH1106::Inverse);
      Display.fill_rect(step * 8 + 3, 25, 2, 2, DUINO_SH1106::Inverse);
      Display.fill_rect(step * 8 + 3, 36, 2, 2, DUINO_SH1106::Inverse);
      Display.fill_rect(step * 8 + 3, 47, 2, 2, DUINO_SH1106::Inverse);
      Display.fill_rect(step * 8 + 3, 58, 2, 5, DUINO_SH1106::Inverse);
    }

    Display.display(step * 8 + 3, step * 8 + 5, 1, 7);
  }

  struct ParameterValues {
    uint8_t pattern[64];
    int8_t step_count;
    int16_t clock_bpm;
    int8_t swing;
  };

  DUINO_WidgetContainer<6> * container_outer_;
  DUINO_WidgetContainer<3> * container_top_;
  DUINO_SaveWidget<ParameterValues> * widget_save_;
  DUINO_DisplayWidget * widget_measures_;
  DUINO_DisplayWidget * widget_clock_;
  DUINO_DisplayWidget * widget_swing_;
  DUINO_MultiDisplayWidget<STEP_MAX> * widgets_patterns_[4];

  volatile int8_t current_step_;
  int8_t displayed_step_;
};

DU_PLSR_Function * function;

void clock_ext_isr() { function->clock_ext_callback(); }
void reset_isr() { function->reset_callback(); }

void clock_callback() { function->clock_clock_callback(); }
void external_callback() { function->clock_external_callback(); }

void measures_scroll_callback(int delta) { function->widget_measures_scroll_callback(delta); }
void clock_scroll_callback(int delta) { function->widget_clock_scroll_callback(delta); }
void swing_scroll_callback(int delta) { function->widget_swing_scroll_callback(delta); }
void patterns_click_callback_0(uint8_t step) { function->widgets_patterns_click_callback(0, step); }
void patterns_click_callback_1(uint8_t step) { function->widgets_patterns_click_callback(1, step); }
void patterns_click_callback_2(uint8_t step) { function->widgets_patterns_click_callback(2, step); }
void patterns_click_callback_3(uint8_t step) { function->widgets_patterns_click_callback(3, step); }

void setup()
{
  function = new DU_PLSR_Function();

  function->begin();
}

void loop()
{
  function->function_loop();
}
