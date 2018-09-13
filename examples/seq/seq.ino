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
 * SC4    [_][^]    SC3
 */

#include <du-ino_function.h>
#include <du-ino_widgets.h>
#include <du-ino_save.h>
#include <du-ino_clock.h>
#include <du-ino_dsp.h>
#include <avr/pgmspace.h>

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

volatile uint8_t stage, step;
volatile bool gate, reverse;

DUINO_Filter * slew_filter;

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
  DU_SEQ_Function() : DUINO_Function(0b01111100) { }
  
  virtual void setup()
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

    last_gate = false;
    last_stage = 0;
    last_diradd_mode = false;
    last_reverse = false;

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
      if (widget_save_->params.vals.stage_pitch[i] < 0)
      {
        widget_save_->params.vals.stage_pitch[i] = 0;
      }
      if (widget_save_->params.vals.stage_pitch[i] > 119)
      {
        widget_save_->params.vals.stage_pitch[i] = 119;
      }
    }

    for (uint8_t i = 0; i < 8; ++i)
    {
      if (widget_save_->params.vals.stage_steps[i] < 1 || widget_save_->params.vals.stage_steps[i] > 8)
      {
        widget_save_->params.vals.stage_steps[i] = 8;
      }
    }
    
    for (uint8_t i = 0; i < 8; ++i)
    {
      if(widget_save_->params.vals.stage_gate[i] < 0 || widget_save_->params.vals.stage_gate[i] > 5)
      {
        widget_save_->params.vals.stage_gate[i] = 2;
      }
    }

    if (widget_save_->params.vals.stage_count < 1 || widget_save_->params.vals.stage_count > 8)
    {
      widget_save_->params.vals.stage_count = 8;
    }
    
    if (widget_save_->params.vals.slew_rate < 0 || widget_save_->params.vals.slew_rate > 16)
    {
      widget_save_->params.vals.slew_rate = 8;
    }
    slew_filter->set_frequency(slew_hz(widget_save_->params.vals.slew_rate));

    if (widget_save_->params.vals.clock_bpm < 0 || widget_save_->params.vals.clock_bpm > 300)
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

    if (widget_save_->params.vals.gate_time < 0 || widget_save_->params.vals.gate_time > 16)
    {
      widget_save_->params.vals.gate_time = 8;
    }
    gate_ms = widget_save_->params.vals.gate_time * (uint16_t)(Clock.get_period() / 8000);

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

  virtual void loop()
  {
    // cache stage, step, and clock gate (so that each loop is "atomic")
    const uint8_t cached_stage = stage;
    const uint8_t cached_step = step;
    cached_clock_gate = Clock.state();
    cached_retrigger = Clock.retrigger();

    if (cached_retrigger)
    {
      // drop gate at start of stage
      if (!cached_step)
      {
        gt_out(GT1, false);
      }

      // drop clock each step
      gt_out(GT2, false);

      // update step clock time
      clock_time = millis();
    }

    // set gate state
    switch ((GateMode)(widget_save_->params.vals.stage_gate[cached_stage]))
    {
      case GATE_NONE:
        gate = false;
        break;
      case GATE_1SHT:
        if (!cached_step)
        {
          gate = partial_gate();
        }
        break;
      case GATE_REPT:
        gate = partial_gate();
        break;
      case GATE_LONG:
        if (cached_step == widget_save_->params.vals.stage_steps[cached_stage] - 1)
        {
          gate = partial_gate();
        }
        else
        {
          gate = true;
        }
        break;
      case GATE_EXT1:
        gate = gt_read(CI3);
        break;
      case GATE_EXT2:
        gate = gt_read(CI4);
        break;
    }

    // set pitch CV state
    const float pitch_cv = note_to_cv(widget_save_->params.vals.stage_pitch[cached_stage]);
    const bool slew_on = (bool)((widget_save_->params.vals.stage_slew >> cached_stage) & 1);
    cv_out(CO1, slew_on ? slew_filter->filter(pitch_cv) : pitch_cv);

    // set gate and clock states
    gt_out(GT1, gate);
    gt_out(GT2, cached_clock_gate);

    // update reverse setting
    if (!widget_save_->params.vals.diradd_mode)
    {
      reverse = gt_read(CI1);
    }

    widget_loop();

    // display reverse/address
    if (widget_save_->params.vals.diradd_mode != last_diradd_mode
        || (!widget_save_->params.vals.diradd_mode && reverse != last_reverse)
        || (widget_save_->params.vals.diradd_mode && stage != last_stage))
    {
      display_reverse_address(30, 12);
      last_diradd_mode = widget_save_->params.vals.diradd_mode;
      last_reverse = reverse;
      Display.display(30, 34, 1, 2);
    }

    // display gate
    if (gate != last_gate || stage != last_stage)
    {
      const uint8_t last_stage_cached = last_stage;
      last_gate = gate;
      last_stage = stage;

      if (gate)
      {
        if (stage != last_stage_cached)
        {
          display_gate(last_stage_cached, DUINO_SH1106::Black);
        }
        display_gate(stage, DUINO_SH1106::White);
      }
      else
      {
        display_gate(last_stage_cached, DUINO_SH1106::Black);
      }

      Display.display(16 * last_stage_cached + 6, 16 * last_stage_cached + 9, 3, 3);
      Display.display(16 * stage + 6, 16 * stage + 9, 3, 3);
    }
  }

  void clock_clock_callback()
  {
    if (Clock.state())
    {
      // TODO: implement half clocking
      step++;
      step %= widget_save_->params.vals.stage_steps[stage];
      if (!step)
      {
        stage = widget_save_->params.vals.diradd_mode ? address_to_stage() : (reverse ?
            (stage ? stage - 1 : widget_save_->params.vals.stage_count - 1) : stage + 1);
        stage %= widget_save_->params.vals.stage_count;
      }
    }
  }

  void clock_external_callback()
  {
    Display.fill_rect(widget_clock_->x() + 1, widget_clock_->y() + 1, 17, 7,
        widget_clock_->inverted() ? DUINO_SH1106::White : DUINO_SH1106::Black);
    display_clock(widget_clock_->x() + 1, widget_clock_->y() + 1, widget_save_->params.vals.clock_bpm,
        widget_clock_->inverted() ? DUINO_SH1106::Black : DUINO_SH1106::White);
    widget_clock_->display();
  }

  void widget_count_scroll_callback(int delta)
  {
    widget_save_->params.vals.stage_count += delta;
    if (widget_save_->params.vals.stage_count < 1)
    {
      widget_save_->params.vals.stage_count = 1;
    }
    else if (widget_save_->params.vals.stage_count > 8)
    {
      widget_save_->params.vals.stage_count = 8;
    }
    widget_save_->mark_changed();
    widget_save_->display();
    Display.fill_rect(widget_count_->x() + 1, widget_count_->y() + 1, 5, 7, DUINO_SH1106::White);
    Display.draw_char(widget_count_->x() + 1, widget_count_->y() + 1, '0' + widget_save_->params.vals.stage_count,
        DUINO_SH1106::Black);
    widget_count_->display();
  }

  void widget_diradd_scroll_callback(int delta)
  {
    if (delta < 0)
    {
      widget_save_->params.vals.diradd_mode = 0;
    }
    else if (delta > 0)
    {
      widget_save_->params.vals.diradd_mode = 1;
    }
    widget_save_->mark_changed();
    widget_save_->display();
    Display.fill_rect(widget_diradd_->x() + 1, widget_diradd_->y() + 1, 5, 7, DUINO_SH1106::White);
    Display.draw_char(widget_diradd_->x() + 1, widget_diradd_->y() + 1,
        widget_save_->params.vals.diradd_mode ? 'A' : 'D', DUINO_SH1106::Black);
    widget_diradd_->display();
  }

  void widget_slew_scroll_callback(int delta)
  {
    widget_save_->params.vals.slew_rate += delta;
    if (widget_save_->params.vals.slew_rate < 0)
    {
      widget_save_->params.vals.slew_rate = 0;
    }
    else if (widget_save_->params.vals.slew_rate > 16)
    {
      widget_save_->params.vals.slew_rate = 16;
    }
    slew_filter->set_frequency(slew_hz(widget_save_->params.vals.slew_rate));
    widget_save_->mark_changed();
    widget_save_->display();
    Display.fill_rect(widget_slew_->x() + 2, widget_slew_->y() + 2, 16, 5, DUINO_SH1106::White);
    display_slew_rate(widget_slew_->x() + 2, widget_slew_->y() + 2, widget_save_->params.vals.slew_rate,
        DUINO_SH1106::Black);
    widget_slew_->display();
  }

  void widget_gate_scroll_callback(int delta)
  {
    widget_save_->params.vals.gate_time += delta;
    if (widget_save_->params.vals.gate_time < 0)
    {
      widget_save_->params.vals.gate_time = 0;
    }
    else if (widget_save_->params.vals.gate_time > 16)
    {
      widget_save_->params.vals.gate_time = 16;
    }
    gate_ms = widget_save_->params.vals.gate_time * (uint16_t)(Clock.get_period() / 8000);
    widget_save_->mark_changed();
    widget_save_->display();
    Display.fill_rect(widget_gate_->x() + 2, widget_gate_->y() + 2, 16, 5, DUINO_SH1106::White);
    display_gate_time(widget_gate_->x() + 2, widget_gate_->y() + 2, widget_save_->params.vals.gate_time,
        DUINO_SH1106::Black);
    widget_gate_->display();
  }

  void widget_clock_scroll_callback(int delta)
  {
    widget_save_->params.vals.clock_bpm += delta;
    if (widget_save_->params.vals.clock_bpm < 0)
    {
      widget_save_->params.vals.clock_bpm = 0;
    }
    else if (widget_save_->params.vals.clock_bpm > 300)
    {
      widget_save_->params.vals.clock_bpm = 300;
    }
    if (widget_save_->params.vals.clock_bpm)
    {
      Clock.set_bpm(widget_save_->params.vals.clock_bpm);
    }
    else
    {
      Clock.set_external();
    }
    gate_ms = widget_save_->params.vals.gate_time * (uint16_t)(Clock.get_period() / 8000);
    widget_save_->mark_changed();
    widget_save_->display();
    Display.fill_rect(widget_clock_->x() + 1, widget_clock_->y() + 1, 17, 7, DUINO_SH1106::White);
    display_clock(widget_clock_->x() + 1, widget_clock_->y() + 1, widget_save_->params.vals.clock_bpm,
        DUINO_SH1106::Black);
    widget_clock_->display();
  }

  void widgets_pitch_scroll_callback(uint8_t stage_selected, int delta)
  {
    widget_save_->params.vals.stage_pitch[stage_selected] += delta;
    if (widget_save_->params.vals.stage_pitch[stage_selected] < 0)
    {
      widget_save_->params.vals.stage_pitch[stage_selected] = 0;
    }
    else if (widget_save_->params.vals.stage_pitch[stage_selected] > 119)
    {
      widget_save_->params.vals.stage_pitch[stage_selected] = 119;
    }
    widget_save_->mark_changed();
    widget_save_->display();
    Display.fill_rect(widgets_pitch_->x(stage_selected), widgets_pitch_->y(stage_selected), 16, 15,
        DUINO_SH1106::White);
    display_note(widgets_pitch_->x(stage_selected), widgets_pitch_->y(stage_selected),
        widget_save_->params.vals.stage_pitch[stage_selected], DUINO_SH1106::Black);
    widgets_pitch_->display();
  }

  void widgets_steps_scroll_callback(uint8_t stage_selected, int delta)
  {
    widget_save_->params.vals.stage_steps[stage_selected] += delta;
    if (widget_save_->params.vals.stage_steps[stage_selected] < 1)
    {
      widget_save_->params.vals.stage_steps[stage_selected] = 1;
    }
    else if (widget_save_->params.vals.stage_steps[stage_selected] > 8)
    {
      widget_save_->params.vals.stage_steps[stage_selected] = 8;
    }
    widget_save_->mark_changed();
    widget_save_->display();
    Display.fill_rect(widgets_steps_->x(stage_selected) + 1, widgets_steps_->y(stage_selected) + 1, 5, 7,
        DUINO_SH1106::White);
    Display.draw_char(widgets_steps_->x(stage_selected) + 1, widgets_steps_->y(stage_selected) + 1,
        '0' + widget_save_->params.vals.stage_steps[stage_selected], DUINO_SH1106::Black);
    widgets_steps_->display();
  }

  void widgets_gate_scroll_callback(uint8_t stage_selected, int delta)
  {
    widget_save_->params.vals.stage_gate[stage_selected] += delta;
    if (widget_save_->params.vals.stage_gate[stage_selected] < 0)
    {
      widget_save_->params.vals.stage_gate[stage_selected] = 0;
    }
    else if (widget_save_->params.vals.stage_gate[stage_selected] > 5)
    {
      widget_save_->params.vals.stage_gate[stage_selected] = 5;
    }
    widget_save_->mark_changed();
    widget_save_->display();
    Display.fill_rect(widgets_gate_->x(stage_selected) + 1, widgets_gate_->y(stage_selected) + 1, 7, 7,
        DUINO_SH1106::White);
    Display.draw_bitmap_7(widgets_gate_->x(stage_selected) + 1, widgets_gate_->y(stage_selected) + 1,
        gate_mode_icons, (GateMode)(widget_save_->params.vals.stage_gate[stage_selected]), DUINO_SH1106::Black);
    widgets_gate_->display();
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
    if (addr_stage < 0)
    {
      addr_stage = 0;
    }
    else if (stage > widget_save_->params.vals.stage_count - 1)
    {
      addr_stage = widget_save_->params.vals.stage_count - 1;
    }
    return (uint8_t)addr_stage;
  }

private:
  bool partial_gate()
  {
    return (Clock.get_external() && cached_clock_gate)
           || cached_retrigger
           || ((millis() - clock_time) < gate_ms);
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
      Display.draw_char(30, 12, '1' + stage, DUINO_SH1106::White);
    }
    else
    {
      Display.draw_char(30, 12, 0x10 + (unsigned char)reverse, DUINO_SH1106::White);
    }
  }

  void display_gate(uint8_t stage, DUINO_SH1106::Color color)
  {
    Display.fill_rect(16 * stage + 6, 26, 4, 4, color);
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

  bool cached_clock_gate, cached_retrigger;
  unsigned long clock_time;

  bool last_gate;
  uint8_t last_stage;
  bool last_diradd_mode;
  bool last_reverse;
  uint16_t gate_ms;
};

DU_SEQ_Function * function;

void clock_ext_isr()
{
  Clock.on_jack(function->gt_read_debounce(DUINO_Function::GT3));
}

void reset_isr()
{
  stage = step = 0;
  Clock.reset();
}

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
  stage = step = 0;
  gate = reverse = false;

  slew_filter = new DUINO_Filter(DUINO_Filter::LowPass, 1.0, 0.0);

  function = new DU_SEQ_Function();

  function->begin();
}

void loop()
{
  function->loop();
}
