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
 * DU-INO DU-SEQ Emulator
 * Aaron Mavrinac <aaron@logick.ca>
 *
 * JACK    FUNCTION
 * ----    --------
 * GT1 O - gate out
 * GT2 O - clock out
 * GT3 I - clock in
 * GT4 I - reset in
 * CI1   - reverse/address in
 * CI2   - half clock in
 * CI3   - external gate 1 in
 * CI4   - external gate 2 in
 * OFFST -
 * CO1   - pitch CV out
 * CO2   -
 * CO3   -
 * CO4   -
 * FNCTN -
 *
 * SWITCH CONFIGURATION
 * --------------------
 * SG2    [_][_]    SG1
 * SG4    [^][^]    SG3
 * SC2    [^][^]    SC1
 * SC4    [^][^]    SC3
 */

#include <du-ino_function.h>
#include <du-ino_widgets.h>
#include <du-ino_save.h>
#include <du-ino_clock.h>
#include <du-ino_dsp.h>
#include <du-ino_utils.h>
#include <avr/pgmspace.h>

#define PITCH_MAX 119
#define STEPS_MIN 1
#define STEPS_MAX 8
#define STAGE_MIN 1
#define STAGE_MAX 8
#define GATE_TIME_MAX 16
#define GATE_TIME_DIV 8000
#define SLEW_RATE_MAX 16
#define CLOCK_BPM_MAX 300

enum GateMode
{
  GATE_NONE = 0,
  GATE_1SHT = 1,
  GATE_REPT = 2,
  GATE_LONG = 3,
  GATE_EXT1 = 4,
  GATE_EXT2 = 5
};

enum Intonation
{
  IN,
  IF,
  IS
};

static const unsigned char gate_mode_icons[] PROGMEM =
{
  0x00, 0x22, 0x14, 0x08, 0x14, 0x22, 0x00,  // off
  0x1C, 0x22, 0x5D, 0x5D, 0x5D, 0x22, 0x1C,  // single
  0x7C, 0x00, 0x7F, 0x00, 0x7F, 0x00, 0x7C,  // multi
  0x08, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x08,  // long
  0x30, 0x40, 0x52, 0x5F, 0x50, 0x40, 0x30,  // ext 1
  0x30, 0x40, 0x59, 0x55, 0x52, 0x40, 0x30   // ext 2
};

static const unsigned char semitone_lt[] PROGMEM = {'C', 'C', 'D', 'E', 'E', 'F', 'F', 'G', 'G', 'A', 'B', 'B'};
static const Intonation semitone_in[] PROGMEM = {IN, IS, IN, IF, IN, IN, IS, IN, IS, IN, IF, IN};

void clock_ext_isr();
void reset_isr();

void clock_callback();
void external_callback();

void count_scroll_callback(int delta);
void diradd_scroll_callback(int delta);
void slew_scroll_callback(int delta);
void gate_scroll_callback(int delta);
void clock_scroll_callback(int delta);
void s_pitch_scroll_callback(uint8_t selected, int delta);
void s_steps_scroll_callback(uint8_t selected, int delta);
void s_gate_scroll_callback(uint8_t selected, int delta);
void s_slew_scroll_callback(uint8_t selected, int delta);
void s_pitch_click_callback();
void s_steps_click_callback();
void s_gate_click_callback();
void s_slew_click_callback();

class DU_SEQ_Function : public DUINO_Function
{
public:
  DU_SEQ_Function() : DUINO_Function(0b11111100) { }
  
  virtual void function_setup()
  {
    // build widget hierarchy
    container_outer_ = new DUINO_WidgetContainer<6>(DUINO_Widget::DoubleClick, 2);
    widget_save_ = new DUINO_SaveWidget<ParameterValues>(121, 0);
    container_outer_->attach_child(widget_save_, 0);
    container_top_ = new DUINO_WidgetContainer<5>(DUINO_Widget::Click);
    widget_count_ = new DUINO_DisplayWidget(9, 11, 7, 9, DUINO_Widget::Full);
    widget_count_->attach_scroll_callback(count_scroll_callback);
    container_top_->attach_child(widget_count_, 0);
    widget_diradd_ = new DUINO_DisplayWidget(21, 11, 7, 9, DUINO_Widget::Full);
    widget_diradd_->attach_scroll_callback(diradd_scroll_callback);
    container_top_->attach_child(widget_diradd_, 1);
    widget_slew_ = new DUINO_DisplayWidget(58, 11, 20, 9, DUINO_Widget::Full);
    widget_slew_->attach_scroll_callback(slew_scroll_callback);
    container_top_->attach_child(widget_slew_, 2);
    widget_gate_ = new DUINO_DisplayWidget(83, 11, 20, 9, DUINO_Widget::Full);
    widget_gate_->attach_scroll_callback(gate_scroll_callback);
    container_top_->attach_child(widget_gate_, 3);
    widget_clock_ = new DUINO_DisplayWidget(108, 11, 19, 9, DUINO_Widget::Full);
    widget_clock_->attach_scroll_callback(clock_scroll_callback);
    container_top_->attach_child(widget_clock_, 4);
    container_outer_->attach_child(container_top_, 1);
    widgets_pitch_ = new DUINO_MultiDisplayWidget<8>(0, 32, 16, 15, 16, false, DUINO_Widget::Full, DUINO_Widget::Click);
    widgets_pitch_->attach_scroll_callback_array(s_pitch_scroll_callback);
    widgets_pitch_->attach_click_callback(s_pitch_click_callback);
    container_outer_->attach_child(widgets_pitch_, 2);
    widgets_steps_ = new DUINO_MultiDisplayWidget<8>(0, 48, 7, 9, 16, false, DUINO_Widget::Full, DUINO_Widget::Click);
    widgets_steps_->attach_scroll_callback_array(s_steps_scroll_callback);
    widgets_steps_->attach_click_callback(s_steps_click_callback);
    container_outer_->attach_child(widgets_steps_, 3);
    widgets_gate_ = new DUINO_MultiDisplayWidget<8>(7, 48, 9, 9, 16, false, DUINO_Widget::Full, DUINO_Widget::Click);
    widgets_gate_->attach_scroll_callback_array(s_gate_scroll_callback);
    widgets_gate_->attach_click_callback(s_gate_click_callback);
    container_outer_->attach_child(widgets_gate_, 4);
    widgets_slew_ = new DUINO_MultiDisplayWidget<8>(0, 58, 16, 6, 16, false, DUINO_Widget::Full, DUINO_Widget::Click);
    widgets_slew_->attach_scroll_callback_array(s_slew_scroll_callback);
    widgets_slew_->attach_click_callback(s_slew_click_callback);
    container_outer_->attach_child(widgets_slew_, 5);

    slew_filter_ = new DUINO_Filter(DUINO_Filter::LowPass, 1.0, 0.0);

    stage_ = step_ = 0;
    gate_ = false;

    last_gate_ = false;
    last_stage_ = 0;
    last_diradd_mode_ = false;
    last_reverse_ = false;
    last_half_clock_ = false;

    reverse_ = false;

    Clock.begin();
    Clock.attach_clock_callback(clock_callback);
    Clock.attach_external_callback(external_callback);

    gt_attach_interrupt(GT3, clock_ext_isr, CHANGE);
    gt_attach_interrupt(GT4, reset_isr, RISING);

    // draw top line
    Display.draw_du_logo_sm(0, 0, DUINO_SH1106::White);
    Display.draw_text(16, 0, "SEQ", DUINO_SH1106::White);

    // draw save box
    Display.fill_rect(widget_save_->x() + 1, widget_save_->y() + 1, 5, 5, DUINO_SH1106::White);

    // load settings
    widget_save_->load_params();

    // verify settings and export to function parameters
    for (uint8_t i = 0; i < 8; ++i)
    {
      widget_save_->params.vals.stage_pitch[i] = clamp<int8_t>(widget_save_->params.vals.stage_pitch[i], 0, PITCH_MAX);

      if (widget_save_->params.vals.stage_steps[i] < STEPS_MIN || widget_save_->params.vals.stage_steps[i] > STEPS_MAX)
      {
        widget_save_->params.vals.stage_steps[i] = STEPS_MAX;
      }

      if(widget_save_->params.vals.stage_gate[i] < GATE_NONE || widget_save_->params.vals.stage_gate[i] > GATE_EXT2)
      {
        widget_save_->params.vals.stage_gate[i] = GATE_REPT;
      }
    }

    if (widget_save_->params.vals.stage_count < STAGE_MIN || widget_save_->params.vals.stage_count > STAGE_MAX)
    {
      widget_save_->params.vals.stage_count = STAGE_MAX;
    }
    
    if (widget_save_->params.vals.slew_rate < 0 || widget_save_->params.vals.slew_rate > SLEW_RATE_MAX)
    {
      widget_save_->params.vals.slew_rate = SLEW_RATE_MAX / 2;
    }
    slew_filter_->set_frequency(slew_hz(widget_save_->params.vals.slew_rate));

    if (widget_save_->params.vals.clock_bpm < 0 || widget_save_->params.vals.clock_bpm > CLOCK_BPM_MAX)
    {
      widget_save_->params.vals.clock_bpm = 0;
    }
    if (widget_save_->params.vals.clock_bpm)
    {
      Clock.set_bpm(widget_save_->params.vals.clock_bpm);
    }
    else
    {
      Clock.set_external();
    }

    if (widget_save_->params.vals.gate_time < 0 || widget_save_->params.vals.gate_time > GATE_TIME_MAX)
    {
      widget_save_->params.vals.gate_time = GATE_TIME_MAX / 2;
    }
    gate_ms_ = widget_save_->params.vals.gate_time * (uint16_t)(Clock.get_period() / GATE_TIME_DIV);

    // draw global elements
    for (uint8_t i = 0; i < 6; ++i)
    {
      Display.draw_vline(2 + i, 18 - i, i + 1, DUINO_SH1106::White);
    }
    Display.draw_char(widget_count_->x() + 1, widget_count_->y() + 1,
        '0' + widget_save_->params.vals.stage_count, DUINO_SH1106::White);
    Display.draw_char(widget_diradd_->x() + 1, widget_diradd_->y() + 1,
        widget_save_->params.vals.diradd_mode ? 'A' : 'D', DUINO_SH1106::White);
    Display.draw_char(30, 12, 0x10, DUINO_SH1106::White);

    Display.draw_vline(widget_slew_->x() + 1, widget_slew_->y() + 1, 7, DUINO_SH1106::White);
    Display.draw_hline(widget_slew_->x() + 2, widget_slew_->y() + 1, 16, DUINO_SH1106::White);
    Display.draw_hline(widget_slew_->x() + 2, widget_slew_->y() + 7, 16, DUINO_SH1106::White);
    Display.draw_vline(widget_slew_->x() + widget_slew_->width() - 2, widget_slew_->y() + 1, 7, DUINO_SH1106::White);
    display_slew_rate(widget_slew_->x() + 2, widget_slew_->y() + 2, widget_save_->params.vals.slew_rate,
        DUINO_SH1106::White);

    Display.draw_vline(widget_gate_->x() + 1, widget_gate_->y() + 1, 7, DUINO_SH1106::White);
    Display.draw_hline(widget_gate_->x() + 2, widget_gate_->y() + 1, 16, DUINO_SH1106::White);
    Display.draw_hline(widget_gate_->x() + 2, widget_gate_->y() + 7, 16, DUINO_SH1106::White);
    Display.draw_vline(widget_gate_->x() + widget_gate_->width() - 2, widget_slew_->y() + 1, 7, DUINO_SH1106::White);   
    display_gate_time(widget_gate_->x() + 2, widget_gate_->y() + 2, widget_save_->params.vals.gate_time,
        DUINO_SH1106::White);

    display_clock(widget_clock_->x() + 1, widget_clock_->y() + 1, widget_save_->params.vals.clock_bpm,
        DUINO_SH1106::White);

    // draw step elements
    for (uint8_t i = 0; i < 8; ++i)
    {
      // pitch
      display_note(widgets_pitch_->x(i), widgets_pitch_->y(i), widget_save_->params.vals.stage_pitch[i],
          DUINO_SH1106::White);
      // steps
      Display.draw_char(widgets_steps_->x(i) + 1, widgets_steps_->y(i) + 1,
          '0' + widget_save_->params.vals.stage_steps[i], DUINO_SH1106::White);
      // gate mode
      Display.draw_bitmap_7(widgets_gate_->x(i) + 1, widgets_gate_->y(i) + 1,
          gate_mode_icons, (GateMode)(widget_save_->params.vals.stage_gate[i]), DUINO_SH1106::White);
      // slew
      Display.fill_rect(widgets_slew_->x(i) + 1, widgets_slew_->y(i) + 1, 14, 4, DUINO_SH1106::White);
      Display.fill_rect(widgets_slew_->x(i) + 2 + 6 * (~(widget_save_->params.vals.stage_slew >> i) & 1),
          widgets_slew_->y(i) + 2, 6, 2, DUINO_SH1106::Black);
    }

    widget_setup(container_outer_);
    Display.display();
  }

  virtual void function_loop()
  {
    // cache stage, step, and clock gate (so that each loop is "atomic")
    const uint8_t cached_stage = stage_;
    const uint8_t cached_step = step_;
    cached_clock_gate_ = Clock.state();
    cached_retrigger_ = Clock.retrigger();

    if (cached_retrigger_)
    {
      // drop gate at start of stage
      if (!cached_step)
      {
        gt_out(GT1, false);
      }

      // drop clock each step
      gt_out(GT2, false);

      // update step clock time
      clock_time_ = millis();
    }

    // set gate state
    switch ((GateMode)(widget_save_->params.vals.stage_gate[cached_stage]))
    {
      case GATE_NONE:
        gate_ = false;
        break;
      case GATE_1SHT:
        if (!cached_step)
        {
          gate_ = partial_gate();
        }
        break;
      case GATE_REPT:
        gate_ = partial_gate();
        break;
      case GATE_LONG:
        if (cached_step == widget_save_->params.vals.stage_steps[cached_stage] - 1)
        {
          gate_ = partial_gate();
        }
        else
        {
          gate_ = true;
        }
        break;
      case GATE_EXT1:
        gate_ = gt_read(CI3);
        break;
      case GATE_EXT2:
        gate_ = gt_read(CI4);
        break;
    }

    // set pitch CV state
    const float pitch_cv = note_to_cv(widget_save_->params.vals.stage_pitch[cached_stage]);
    const bool slew_on = (bool)((widget_save_->params.vals.stage_slew >> cached_stage) & 1);
    cv_out(CO1, slew_on ? slew_filter_->filter(pitch_cv) : pitch_cv);

    // set gate and clock states
    gt_out(GT1, gate_);
    gt_out(GT2, cached_clock_gate_);

    // update reverse setting
    if (!widget_save_->params.vals.diradd_mode)
    {
      reverse_ = gt_read(CI1);
    }

    // update half clock setting
    Clock.set_divider(gt_read(CI2) ? 2 : 1);

    widget_loop();

    // display reverse/address
    if (widget_save_->params.vals.diradd_mode != last_diradd_mode_
        || (!widget_save_->params.vals.diradd_mode && reverse_ != last_reverse_)
        || (widget_save_->params.vals.diradd_mode && stage_ != last_stage_))
    {
      display_reverse_address(30, 12);
      last_diradd_mode_ = widget_save_->params.vals.diradd_mode;
      last_reverse_ = reverse_;
      Display.display(30, 34, 1, 2);
    }

    // display half clock
    const bool half_clock = Clock.get_divider() == 2;
    if (half_clock != last_half_clock_)
    {
      display_half_clock(41, 12, half_clock);
      last_half_clock_ = half_clock;
      Display.display(41, 51, 1, 2);
    }

    // display gate
    if (gate_ != last_gate_ || stage_ != last_stage_)
    {
      const uint8_t last_stage_cached = last_stage_;
      last_gate_ = gate_;
      last_stage_ = stage_;

      if (gate_)
      {
        if (stage_ != last_stage_cached)
        {
          display_gate(last_stage_cached, DUINO_SH1106::Black);
        }
        display_gate(stage_, DUINO_SH1106::White);
      }
      else
      {
        display_gate(last_stage_cached, DUINO_SH1106::Black);
      }

      Display.display(16 * last_stage_cached + 6, 16 * last_stage_cached + 9, 3, 3);
      Display.display(16 * stage_ + 6, 16 * stage_ + 9, 3, 3);
    }
  }

  void clock_ext_callback()
  {
    Clock.on_jack(gt_read_debounce(DUINO_Function::GT3));
  }

  void reset_callback()
  {
    stage_ = step_ = 0;
    Clock.reset();
  }

  void clock_clock_callback()
  {
    if (Clock.state())
    {
      step_++;
      step_ %= widget_save_->params.vals.stage_steps[stage_];
      if (!step_)
      {
        stage_ = widget_save_->params.vals.diradd_mode ? address_to_stage() : (reverse_ ?
            (stage_ ? stage_ - 1 : widget_save_->params.vals.stage_count - 1) : stage_ + 1);
        stage_ %= widget_save_->params.vals.stage_count;
      }
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

  void widget_count_scroll_callback(int delta)
  {
    if (adjust<int8_t>(widget_save_->params.vals.stage_count, delta, STAGE_MIN, STAGE_MAX))
    {
      widget_save_->mark_changed();
      widget_save_->display();
      Display.fill_rect(widget_count_->x() + 1, widget_count_->y() + 1, 5, 7, DUINO_SH1106::White);
      Display.draw_char(widget_count_->x() + 1, widget_count_->y() + 1, '0' + widget_save_->params.vals.stage_count,
          DUINO_SH1106::Black);
      widget_count_->display();
    }
  }

  void widget_diradd_scroll_callback(int delta)
  {
    widget_save_->params.vals.diradd_mode = delta < 0 ? 0 : 1;
    widget_save_->mark_changed();
    widget_save_->display();
    Display.fill_rect(widget_diradd_->x() + 1, widget_diradd_->y() + 1, 5, 7, DUINO_SH1106::White);
    Display.draw_char(widget_diradd_->x() + 1, widget_diradd_->y() + 1,
        widget_save_->params.vals.diradd_mode ? 'A' : 'D', DUINO_SH1106::Black);
    widget_diradd_->display();
  }

  void widget_slew_scroll_callback(int delta)
  {
    if (adjust<int8_t>(widget_save_->params.vals.slew_rate, delta, 0, SLEW_RATE_MAX))
    {
      slew_filter_->set_frequency(slew_hz(widget_save_->params.vals.slew_rate));
      widget_save_->mark_changed();
      widget_save_->display();
      Display.fill_rect(widget_slew_->x() + 2, widget_slew_->y() + 2, 16, 5, DUINO_SH1106::White);
      display_slew_rate(widget_slew_->x() + 2, widget_slew_->y() + 2, widget_save_->params.vals.slew_rate,
          DUINO_SH1106::Black);
      widget_slew_->display();
    }
  }

  void widget_gate_scroll_callback(int delta)
  {
    if (adjust<int8_t>(widget_save_->params.vals.gate_time, delta, 0, GATE_TIME_MAX))
    {
      gate_ms_ = widget_save_->params.vals.gate_time * (uint16_t)(Clock.get_period() / GATE_TIME_DIV);
      widget_save_->mark_changed();
      widget_save_->display();
      Display.fill_rect(widget_gate_->x() + 2, widget_gate_->y() + 2, 16, 5, DUINO_SH1106::White);
      display_gate_time(widget_gate_->x() + 2, widget_gate_->y() + 2, widget_save_->params.vals.gate_time,
          DUINO_SH1106::Black);
      widget_gate_->display();
    }
  }

  void widget_clock_scroll_callback(int delta)
  {
    if (adjust<int16_t>(widget_save_->params.vals.clock_bpm, delta, 0, CLOCK_BPM_MAX))
    {
      if (widget_save_->params.vals.clock_bpm)
      {
        Clock.set_bpm(widget_save_->params.vals.clock_bpm);
      }
      else
      {
        Clock.set_external();
      }
      gate_ms_ = widget_save_->params.vals.gate_time * (uint16_t)(Clock.get_period() / GATE_TIME_DIV);
      widget_save_->mark_changed();
      widget_save_->display();
      Display.fill_rect(widget_clock_->x() + 1, widget_clock_->y() + 1, 17, 7, DUINO_SH1106::White);
      display_clock(widget_clock_->x() + 1, widget_clock_->y() + 1, widget_save_->params.vals.clock_bpm,
          DUINO_SH1106::Black);
      widget_clock_->display();
    }
  }

  void widgets_pitch_scroll_callback(uint8_t stage_selected, int delta)
  {
    if (adjust<int8_t>(widget_save_->params.vals.stage_pitch[stage_selected], delta, 0, PITCH_MAX))
    {
      widget_save_->mark_changed();
      widget_save_->display();
      Display.fill_rect(widgets_pitch_->x(stage_selected), widgets_pitch_->y(stage_selected), 16, 15,
          DUINO_SH1106::White);
      display_note(widgets_pitch_->x(stage_selected), widgets_pitch_->y(stage_selected),
          widget_save_->params.vals.stage_pitch[stage_selected], DUINO_SH1106::Black);
      widgets_pitch_->display();
    }
  }

  void widgets_steps_scroll_callback(uint8_t stage_selected, int delta)
  {
    if (adjust<int8_t>(widget_save_->params.vals.stage_steps[stage_selected], delta, STEPS_MIN, STEPS_MAX))
    {
      widget_save_->mark_changed();
      widget_save_->display();
      Display.fill_rect(widgets_steps_->x(stage_selected) + 1, widgets_steps_->y(stage_selected) + 1, 5, 7,
          DUINO_SH1106::White);
      Display.draw_char(widgets_steps_->x(stage_selected) + 1, widgets_steps_->y(stage_selected) + 1,
          '0' + widget_save_->params.vals.stage_steps[stage_selected], DUINO_SH1106::Black);
      widgets_steps_->display();
    }
  }

  void widgets_gate_scroll_callback(uint8_t stage_selected, int delta)
  {
    if (adjust<int8_t>(widget_save_->params.vals.stage_gate[stage_selected], delta, GATE_NONE, GATE_EXT2))
    {
      widget_save_->mark_changed();
      widget_save_->display();
      Display.fill_rect(widgets_gate_->x(stage_selected) + 1, widgets_gate_->y(stage_selected) + 1, 7, 7,
          DUINO_SH1106::White);
      Display.draw_bitmap_7(widgets_gate_->x(stage_selected) + 1, widgets_gate_->y(stage_selected) + 1,
          gate_mode_icons, (GateMode)(widget_save_->params.vals.stage_gate[stage_selected]), DUINO_SH1106::Black);
      widgets_gate_->display();
    }
  }

  void widgets_slew_scroll_callback(uint8_t stage_selected, int delta)
  {
    if (delta < 0)
    {
      widget_save_->params.vals.stage_slew &= ~(1 << stage_selected);
    }
    else if (delta > 0)
    {
      widget_save_->params.vals.stage_slew |= (1 << stage_selected);
    }
    widget_save_->mark_changed();
    widget_save_->display();
    Display.fill_rect(widgets_slew_->x(stage_selected) + 1, widgets_slew_->y(stage_selected) + 1, 14, 4,
        DUINO_SH1106::Black);
    Display.fill_rect(widgets_slew_->x(stage_selected) + 2 + 6 *
        (~(widget_save_->params.vals.stage_slew >> stage_selected) & 1),
        widgets_slew_->y(stage_selected) + 2, 6, 2, DUINO_SH1106::White);
    widgets_slew_->display();
  }

  void widgets_pitch_click_callback()
  {
    widgets_steps_->select(widgets_pitch_->selected());
    widgets_gate_->select(widgets_pitch_->selected());
    widgets_slew_->select(widgets_pitch_->selected());
  }

  void widgets_steps_click_callback()
  {
    widgets_pitch_->select(widgets_steps_->selected());
    widgets_gate_->select(widgets_steps_->selected());
    widgets_slew_->select(widgets_steps_->selected());
  }

  void widgets_gate_click_callback()
  {
    widgets_pitch_->select(widgets_gate_->selected());
    widgets_steps_->select(widgets_gate_->selected());
    widgets_slew_->select(widgets_gate_->selected());
  }

  void widgets_slew_click_callback()
  {
    widgets_pitch_->select(widgets_slew_->selected());
    widgets_steps_->select(widgets_slew_->selected());
    widgets_gate_->select(widgets_slew_->selected());
  }

  uint8_t address_to_stage()
  {
    int8_t addr_stage = (int8_t)(cv_read(CI1) * 1.6);
    return (uint8_t)clamp<int8_t>(addr_stage, 0, widget_save_->params.vals.stage_count - 1);
  }

private:
  bool partial_gate()
  {
    return (Clock.get_external() && cached_clock_gate_)
           || cached_retrigger_
           || ((millis() - clock_time_) < gate_ms_);
  }

  float note_to_cv(int8_t note)
  {
    return ((float)note - 36.0) / 12.0;
  }

  float slew_hz(uint8_t slew_rate)
  {
    if (slew_rate)
    {
      return (float)(17 - slew_rate) / 4.0;
    }
    else
    {
      return 65536.0;
    }
  }

  void display_slew_rate(int16_t x, int16_t y, uint8_t rate, DUINO_SH1106::Color color)
  {
    Display.draw_vline(x + rate - 1, y, 5, color);
  }

  void display_gate_time(int16_t x, int16_t y, uint8_t time, DUINO_SH1106::Color color)
  {
    if (time > 1)
    {
      Display.draw_hline(x, y + 1, time - 1, color);
    }
    Display.draw_vline(x + time - 1, y + 1, 3, color);
    if (time < 16)
    {
      Display.draw_hline(x + time, y + 3, 16 - time, color);
    }
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

  void display_note(int16_t x, int16_t y, int8_t note, DUINO_SH1106::Color color)
  {
    // draw octave
    Display.draw_char(x + 9, y + 7, '0' + note / 12, color);

    // draw note
    Display.draw_char(x + 2, y + 4, pgm_read_byte(&semitone_lt[note % 12]), color);
    
    // draw intonation symbol
    switch (pgm_read_byte(&semitone_in[note % 12]))
    {
      case IF:
        Display.draw_vline(x + 9, y + 1, 5, color);
        Display.draw_pixel(x + 10, y + 3, color);
        Display.draw_pixel(x + 10, y + 5, color);
        Display.draw_vline(x + 11, y + 4, 2, color);
        break;
      case IS:
        Display.draw_vline(x + 9, y + 1, 5, color);
        Display.draw_pixel(x + 10, y + 2, color);
        Display.draw_pixel(x + 10, y + 4, color);
        Display.draw_vline(x + 11, y + 1, 5, color);
        break;
    }
  }

  void display_reverse_address(int16_t x, int16_t y)
  {
    Display.fill_rect(x, y, 5, 7, DUINO_SH1106::Black);

    if (widget_save_->params.vals.diradd_mode)
    {
      Display.draw_char(30, 12, '1' + stage_, DUINO_SH1106::White);
    }
    else
    {
      Display.draw_char(30, 12, 0x10 + (unsigned char)reverse_, DUINO_SH1106::White);
    }
  }

  void display_half_clock(int16_t x, int16_t y, bool on)
  {
    Display.fill_rect(x, y, 11, 7, DUINO_SH1106::Black);

    if (on)
    {
      Display.draw_char(x, y, 0xF6, DUINO_SH1106::White);
      Display.draw_char(x + 6, y, '2', DUINO_SH1106::White);
    }
  }

  void display_gate(uint8_t stage_, DUINO_SH1106::Color color)
  {
    Display.fill_rect(16 * stage_ + 6, 26, 4, 4, color);
  }

  struct ParameterValues
  {
    int8_t stage_pitch[8];
    int8_t stage_steps[8];
    int8_t stage_gate[8];
    uint8_t stage_slew;
    int8_t stage_count;
    uint8_t diradd_mode;
    int8_t slew_rate;
    int8_t gate_time;
    int16_t clock_bpm;
  };

  DUINO_WidgetContainer<6> * container_outer_;
  DUINO_WidgetContainer<5> * container_top_;
  DUINO_SaveWidget<ParameterValues> * widget_save_;
  DUINO_DisplayWidget * widget_count_;
  DUINO_DisplayWidget * widget_diradd_;
  DUINO_DisplayWidget * widget_slew_;
  DUINO_DisplayWidget * widget_gate_;
  DUINO_DisplayWidget * widget_clock_;
  DUINO_MultiDisplayWidget<8> * widgets_pitch_;
  DUINO_MultiDisplayWidget<8> * widgets_steps_;
  DUINO_MultiDisplayWidget<8> * widgets_gate_;
  DUINO_MultiDisplayWidget<8> * widgets_slew_;

  DUINO_Filter * slew_filter_;

  volatile uint8_t stage_, step_;
  volatile bool gate_;

  bool cached_clock_gate_, cached_retrigger_;
  unsigned long clock_time_;

  bool last_gate_;
  uint8_t last_stage_;
  bool last_diradd_mode_;
  bool last_reverse_;
  bool last_half_clock_;

  bool reverse_;

  uint16_t gate_ms_;
};

DU_SEQ_Function * function;

void clock_ext_isr() { function->clock_ext_callback(); }
void reset_isr() { function->reset_callback(); }

void clock_callback() { function->clock_clock_callback(); }
void external_callback() { function->clock_external_callback(); }

void count_scroll_callback(int delta) { function->widget_count_scroll_callback(delta); }
void diradd_scroll_callback(int delta) { function->widget_diradd_scroll_callback(delta); }
void slew_scroll_callback(int delta) { function->widget_slew_scroll_callback(delta); }
void gate_scroll_callback(int delta) { function->widget_gate_scroll_callback(delta); }
void clock_scroll_callback(int delta) { function->widget_clock_scroll_callback(delta); }
void s_pitch_scroll_callback(uint8_t selected, int delta) { function->widgets_pitch_scroll_callback(selected, delta); }
void s_steps_scroll_callback(uint8_t selected, int delta) { function->widgets_steps_scroll_callback(selected, delta); }
void s_gate_scroll_callback(uint8_t selected, int delta) { function->widgets_gate_scroll_callback(selected, delta); }
void s_slew_scroll_callback(uint8_t selected, int delta) { function->widgets_slew_scroll_callback(selected, delta); }
void s_pitch_click_callback() { function->widgets_pitch_click_callback(); }
void s_steps_click_callback() { function->widgets_steps_click_callback(); }
void s_gate_click_callback() { function->widgets_gate_click_callback(); }
void s_slew_click_callback() { function->widgets_slew_click_callback(); }

void setup()
{
  function = new DU_SEQ_Function();

  function->begin();
}

void loop()
{
  function->function_loop();
}
