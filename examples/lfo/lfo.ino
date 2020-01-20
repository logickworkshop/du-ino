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
 * DU-INO Quad LFO
 * Aaron Mavrinac <aaron@logick.ca>
 *
 * JACK    FUNCTION
 * ----    --------
 * GT1 I - hold in
 * GT2 I - hold on zero in
 * GT3 I - sync/tap in
 * GT4 I - reset in
 * CI1   - LFO1 frequency CV
 * CI2   - LFO2 frequency CV
 * CI3   - LFO3 frequency CV
 * CI4   - LFO4 frequency CV
 * OFFST -
 * CO1   - LFO1 out
 * CO2   - LFO2 out
 * CO3   - LFO3 out
 * CO4   - LFO4 out
 * FNCTN - (LFO1 - LFO2) * (LFO3 - LFO4)
 *
 * SWITCH CONFIGURATION
 * --------------------
 * SG2    [^][^]    SG1
 * SG4    [^][^]    SG3
 * SC2    [^][^]    SC1
 * SC4    [^][^]    SC3
 */

#include <du-ino_function.h>
#include <du-ino_widgets.h>
#include <du-ino_indicators.h>
#include <du-ino_save.h>
#include <du-ino_utils.h>
#include <avr/pgmspace.h>

#define AMPLITUDE_MAX 10

enum Waveform
{
  WF_SQUARE = 0,
  WF_SINE = 1,
  WF_SAW = 2,
  WF_TRIANGLE = 3
};

static const unsigned char icons[] PROGMEM =
{
  0x3e, 0x02, 0x02, 0x3e, 0x20, 0x20, 0x3e,  // square wave
  0x1c, 0x02, 0x02, 0x1c, 0x20, 0x20, 0x1c,  // sine wave
  0x20, 0x10, 0x08, 0x04, 0x3e, 0x10, 0x08,  // saw wave
  0x10, 0x08, 0x04, 0x02, 0x04, 0x08, 0x10,  // triangle wave
  0x3e, 0x6f, 0x59, 0x5f, 0x59, 0x6f, 0x3e,  // normal
  0x3e, 0x7b, 0x4d, 0x7d, 0x4d, 0x7b, 0x3e   // inverted
};

void s_waveform_scroll_callback(uint8_t selected, int delta);
void s_range_scroll_callback(uint8_t selected, int delta);
void s_source_scroll_callback(uint8_t selected, int delta);
void s_polarity_scroll_callback(uint8_t selected, int delta);
void s_amplitude_scroll_callback(uint8_t selected, int delta);
void s_invert_scroll_callback(uint8_t selected, int delta);
void s_waveform_click_callback();
void s_range_click_callback();
void s_source_click_callback();
void s_polarity_click_callback();
void s_amplitude_click_callback();
void s_invert_click_callback();

class DU_LFO_Function : public DUINO_Function
{
public:
  DU_LFO_Function() : DUINO_Function(0b11111111) { }

  virtual void function_setup()
  {
    // build widget hierarchy
    container_outer_ = new DUINO_WidgetContainer<7>(DUINO_Widget::DoubleClick, 1);
    widget_save_ = new DUINO_SaveWidget<ParameterValues>(121, 0);
    container_outer_->attach_child(widget_save_, 0);
    widgets_waveform_ = new DUINO_MultiDisplayWidget<4>(14, 13, 9, 9, 13, true, DUINO_Widget::Full,
        DUINO_Widget::Click);
    widgets_waveform_->attach_scroll_callback_array(s_waveform_scroll_callback);
    widgets_waveform_->attach_click_callback(s_waveform_click_callback);
    container_outer_->attach_child(widgets_waveform_, 1);
    widgets_range_ = new DUINO_MultiDisplayWidget<4>(29, 13, 19, 9, 13, true, DUINO_Widget::Full, DUINO_Widget::Click);
    widgets_range_->attach_scroll_callback_array(s_range_scroll_callback);
    widgets_range_->attach_click_callback(s_range_click_callback);
    container_outer_->attach_child(widgets_range_, 2);
    widgets_source_ = new DUINO_MultiDisplayWidget<4>(54, 13, 19, 9, 13, true, DUINO_Widget::Full, DUINO_Widget::Click);
    widgets_source_->attach_scroll_callback_array(s_source_scroll_callback);
    widgets_source_->attach_click_callback(s_source_click_callback);
    container_outer_->attach_child(widgets_source_, 3);
    widgets_polarity_ = new DUINO_MultiDisplayWidget<4>(79, 13, 7, 9, 13, true, DUINO_Widget::Full,
        DUINO_Widget::Click);
    widgets_polarity_->attach_scroll_callback_array(s_polarity_scroll_callback);
    widgets_polarity_->attach_click_callback(s_polarity_click_callback);
    container_outer_->attach_child(widgets_polarity_, 4);
    widgets_amplitude_ = new DUINO_MultiDisplayWidget<4>(86, 13, 12, 9, 13, true, DUINO_Widget::Full,
        DUINO_Widget::Click);
    widgets_amplitude_->attach_scroll_callback_array(s_amplitude_scroll_callback);
    widgets_amplitude_->attach_click_callback(s_amplitude_click_callback);
    container_outer_->attach_child(widgets_amplitude_, 5);
    widgets_invert_ = new DUINO_MultiDisplayWidget<4>(104, 13, 9, 9, 13, true, DUINO_Widget::Full, DUINO_Widget::Click);
    widgets_invert_->attach_scroll_callback_array(s_invert_scroll_callback);
    widgets_invert_->attach_click_callback(s_invert_click_callback);
    container_outer_->attach_child(widgets_invert_, 6);

    // hold indicators
    for(uint8_t i  = 0; i < 4; ++i)
    {
      indicator_hold_[i] = new DUINO_JackIndicator(121, 15 + 13 * i);
    }

    // load settings
    widget_save_->load_params();

    for (uint8_t i = 0; i < 4; ++i)
    {
      widget_save_->params.vals.waveform[i] =
          clamp<int8_t>(widget_save_->params.vals.waveform[i], WF_SQUARE, WF_TRIANGLE);
      widget_save_->params.vals.amplitude[i] = clamp<int8_t>(widget_save_->params.vals.amplitude[i], 0, AMPLITUDE_MAX);
    }

    // draw title
    Display.draw_du_logo_sm(0, 0, DUINO_SH1106::White);
    Display.draw_text(16, 0, "4XLFO", DUINO_SH1106::White);

    // draw save box
    Display.fill_rect(122, 1, 5, 5, DUINO_SH1106::White);

    // draw LFO labels
    Display.draw_char(2, 14, '1', DUINO_SH1106::White);
    Display.draw_char(2, 27, '2', DUINO_SH1106::White);
    Display.draw_char(2, 40, '3', DUINO_SH1106::White);
    Display.draw_char(2, 53, '4', DUINO_SH1106::White);

    widget_setup(container_outer_);
    Display.display();
  }

  virtual void function_loop()
  {
  	widget_loop();
  }

  void widgets_waveform_scroll_callback(uint8_t stage_selected, int delta)
  {
  }

  void widgets_range_scroll_callback(uint8_t stage_selected, int delta)
  {
  }

  void widgets_source_scroll_callback(uint8_t stage_selected, int delta)
  {
  }

  void widgets_polarity_scroll_callback(uint8_t stage_selected, int delta)
  {
  }

  void widgets_amplitude_scroll_callback(uint8_t stage_selected, int delta)
  {
  }

  void widgets_invert_scroll_callback(uint8_t stage_selected, int delta)
  {
  }

  void widgets_waveform_click_callback()
  {
    widgets_range_->select(widgets_waveform_->selected());
    widgets_source_->select(widgets_waveform_->selected());
    widgets_polarity_->select(widgets_waveform_->selected());
    widgets_amplitude_->select(widgets_waveform_->selected());
    widgets_invert_->select(widgets_waveform_->selected());
  }

  void widgets_range_click_callback()
  {
  	widgets_waveform_->select(widgets_range_->selected());
    widgets_source_->select(widgets_range_->selected());
    widgets_polarity_->select(widgets_range_->selected());
    widgets_amplitude_->select(widgets_range_->selected());
    widgets_invert_->select(widgets_range_->selected());
  }

  void widgets_source_click_callback()
  {
  	widgets_waveform_->select(widgets_source_->selected());
    widgets_range_->select(widgets_source_->selected());
    widgets_polarity_->select(widgets_source_->selected());
    widgets_amplitude_->select(widgets_source_->selected());
    widgets_invert_->select(widgets_source_->selected());
  }

  void widgets_polarity_click_callback()
  {
  	widgets_waveform_->select(widgets_polarity_->selected());
    widgets_range_->select(widgets_polarity_->selected());
    widgets_source_->select(widgets_polarity_->selected());
    widgets_amplitude_->select(widgets_polarity_->selected());
    widgets_invert_->select(widgets_polarity_->selected());
  }

  void widgets_amplitude_click_callback()
  {
  	widgets_waveform_->select(widgets_amplitude_->selected());
    widgets_range_->select(widgets_amplitude_->selected());
    widgets_source_->select(widgets_amplitude_->selected());
    widgets_polarity_->select(widgets_amplitude_->selected());
    widgets_invert_->select(widgets_amplitude_->selected());
  }

  void widgets_invert_click_callback()
  {
  	widgets_waveform_->select(widgets_invert_->selected());
    widgets_range_->select(widgets_invert_->selected());
    widgets_source_->select(widgets_invert_->selected());
    widgets_polarity_->select(widgets_invert_->selected());
    widgets_amplitude_->select(widgets_invert_->selected());
  }

private:
  struct ParameterValues
  {
    int8_t waveform[4];
    int8_t range[4];
    int8_t source[4];
    uint8_t polarity : 4;
    int8_t amplitude[4];
    uint8_t invert : 4;
  };

  DUINO_WidgetContainer<7> * container_outer_;
  DUINO_SaveWidget<ParameterValues> * widget_save_;
  DUINO_MultiDisplayWidget<4> * widgets_waveform_;
  DUINO_MultiDisplayWidget<4> * widgets_range_;
  DUINO_MultiDisplayWidget<4> * widgets_source_;
  DUINO_MultiDisplayWidget<4> * widgets_polarity_;
  DUINO_MultiDisplayWidget<4> * widgets_amplitude_;
  DUINO_MultiDisplayWidget<4> * widgets_invert_;

  DUINO_JackIndicator * indicator_hold_[4];
};

DU_LFO_Function * function;

void s_waveform_scroll_callback(uint8_t selected, int delta)
    { function->widgets_waveform_scroll_callback(selected, delta); }
void s_range_scroll_callback(uint8_t selected, int delta)
    { function->widgets_range_scroll_callback(selected, delta); }
void s_source_scroll_callback(uint8_t selected, int delta)
    { function->widgets_source_scroll_callback(selected, delta); }
void s_polarity_scroll_callback(uint8_t selected, int delta)
    { function->widgets_source_scroll_callback(selected, delta); }
void s_amplitude_scroll_callback(uint8_t selected, int delta)
    { function->widgets_amplitude_scroll_callback(selected, delta); }
void s_invert_scroll_callback(uint8_t selected, int delta)
    { function->widgets_invert_scroll_callback(selected, delta); }
void s_waveform_click_callback() { function->widgets_waveform_click_callback(); }
void s_range_click_callback() { function->widgets_range_click_callback(); }
void s_source_click_callback() { function->widgets_source_click_callback(); }
void s_polarity_click_callback() { function->widgets_source_click_callback(); }
void s_amplitude_click_callback() { function->widgets_amplitude_click_callback(); }
void s_invert_click_callback() { function->widgets_invert_click_callback(); }

void setup()
{
  function = new DU_LFO_Function();

  function->begin();
}

void loop()
{
  function->function_loop();
}
