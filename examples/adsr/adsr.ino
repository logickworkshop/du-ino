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
 * DU-INO ADSR Envelope & VCA Function
 * Aaron Mavrinac <aaron@logick.ca>
 *
 * JACK    FUNCTION
 * ----    --------
 * GT1 O - 
 * GT2 O - 
 * GT3 I - gate in
 * GT4 I - toggle envelope trigger in
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
#include <du-ino_widgets.h>
#include <du-ino_save.h>
#include <du-ino_utils.h>
#include <avr/pgmspace.h>

#define ENV_PEAK              10.0 // V
#define ENV_SATURATION        1.581976
#define ENV_DECAY_COEFF       -2.0
#define ENV_RELEASE_COEFF     -2.0
#define ENV_RELEASE_HOLD      3

#define V_MAX                 43

#define DEBOUNCE_MS           100 // ms

static const unsigned char icons[] PROGMEM =
{
  0x38, 0x78, 0x7f, 0x7f, 0x7f, 0x78, 0x38  // gate
};

struct DU_ADSR_Values
{
  uint16_t A;  // ms
  uint16_t D;  // ms
  float S;     // V
  uint16_t R;  // ms
};

static const unsigned char label[4] = {'A', 'D', 'S', 'R'};

void gate_isr();
void switch_isr();

void adsr_scroll_callback(uint8_t selected, int delta);

class DU_ADSR_Function : public DUINO_Function
{
public:
  DU_ADSR_Function() : DUINO_Function(0b10111100) { }
  
  virtual void function_setup()
  {
    // build widget hierarchy
    container_outer_ = new DUINO_WidgetContainer<2>(DUINO_Widget::DoubleClick, 1);
    widget_save_ = new DUINO_SaveWidget<ParameterValues>(121, 0);
    container_outer_->attach_child(widget_save_, 0);
    container_adsr_ = new DUINO_WidgetContainer<8>(DUINO_Widget::Click);
    for (uint8_t i = 0; i < 4; ++i)
    {
      widgets_adsr_[i] = new DUINO_DisplayWidget(11 * i + 2, 55, 7, 9, DUINO_Widget::Full);
      widgets_adsr_[i + 4] = new DUINO_DisplayWidget(11 * i + 86, 55, 7, 9, DUINO_Widget::Full);
      container_adsr_->attach_child(widgets_adsr_[i], i);
      container_adsr_->attach_child(widgets_adsr_[i + 4], i + 4);
    }
    container_outer_->attach_child(container_adsr_, 1);
    container_adsr_->attach_scroll_callback_array(adsr_scroll_callback);

    gate_ = retrigger_ = false;
    selected_env_ = 0;
    debounce_ = 0;
    env_ = 0;
    gate_time_ = 0;
    release_time_ = 0;
    last_selected_env_ = 0;
    last_gate_ = false;

    gt_attach_interrupt(GT3, gate_isr, CHANGE);
    gt_attach_interrupt(GT4, switch_isr, FALLING);

    // load/initialize ADSR values
    widget_save_->load_params();
    for (uint8_t i = 0; i < 8; ++i)
    {
      widget_save_->params.vals.v[i] = clamp<int8_t>(widget_save_->params.vals.v[i], 0, V_MAX);
    }
    for (uint8_t e = 0; e < 2; ++e)
    {
      adsr_values_[e].A = uint16_t(widget_save_->params.vals.v[2 * e]) * 24;
      adsr_values_[e].D = uint16_t(widget_save_->params.vals.v[2 * e + 1]) * 24;
      adsr_values_[e].S = (float(widget_save_->params.vals.v[2 * e + 2]) / float(V_MAX)) * ENV_PEAK;
      adsr_values_[e].R = uint16_t(widget_save_->params.vals.v[2 * e + 3]) * 24;
    }

    // draw title
    Display.draw_du_logo_sm(0, 0, DUINO_SH1106::White);
    Display.draw_text(16, 0, "ADSR/VCA", DUINO_SH1106::White);

    // draw save box
    Display.fill_rect(122, 1, 5, 5, DUINO_SH1106::White);

    // draw envelope indicators
    Display.draw_char(47, 56, 0x11, DUINO_SH1106::White);
    Display.draw_char(54, 56, '1', DUINO_SH1106::White);
    Display.draw_char(69, 56, '2', DUINO_SH1106::White);
    Display.draw_char(76, 56, 0x10, DUINO_SH1106::White);
    Display.fill_rect(53, 55, 7, 9, DUINO_SH1106::Inverse);

    // draw sliders & labels
    for (uint8_t i = 0; i < 8; ++i)
    {
      Display.fill_rect(widgets_adsr_[i]->x() - 1, 51 - widget_save_->params.vals.v[i], 9, 3, DUINO_SH1106::White);
      Display.draw_char(widgets_adsr_[i]->x() + 1, widgets_adsr_[i]->y() + 1, label[i % 4], DUINO_SH1106::White);
    }

    widget_setup(container_outer_);
    Display.display();
  }

  virtual void function_loop()
  {
    if (retrigger_)
    {
      // update the currently selected envelope (we use the same envelope for the duration of a curve)
      env_ = selected_env_;

      // reset the gate and release times so that the curve logic can execute
      gate_time_ = 0;
      release_time_ = 0;

      // reset to wait for another retrigger
      retrigger_ = false;
    }

    if (gate_time_)
    {
      if (gate_)
      {
        // if the gate is active, we are in the attack, decay, or sustain part of the curve, depending on elapsed time
        uint16_t elapsed = millis() - gate_time_;
        if (elapsed < adsr_values_[env_].A)
        {
          // attack
          cv_current_ = ENV_PEAK * ENV_SATURATION * (1.0 - exp(-(float(elapsed) / float(adsr_values_[env_].A))));
        }
        else
        {
          // decay/sustain (no dedicated sustain; the decay curve approaches the sustain value after the decay time)
          cv_current_ = adsr_values_[env_].S + (ENV_PEAK - adsr_values_[env_].S)
              * exp(ENV_DECAY_COEFF * (float(elapsed - adsr_values_[env_].A) / float(adsr_values_[env_].D)));
        }
      }
      else
      {
        if (release_time_)
        {
          uint16_t elapsed = millis() - release_time_;
          if(elapsed < adsr_values_[env_].R * ENV_RELEASE_HOLD)
          {
            // release
            cv_current_ = exp(ENV_RELEASE_COEFF * (float(elapsed) / float(adsr_values_[env_].R))) * cv_released_;
          }
          else
          {
            // release curve has finished; zero output and reset gate and release times
            cv_current_ = 0.0;
            release_time_ = 0;
            gate_time_ = 0;
          }
        }
        else
        {
          // initial release step; record the release time and the initial CV value at release
          release_time_ = millis();
          cv_released_ = cv_current_;
        }
      }

      // output the 10V and 5V envelope outputs on CO1 and CO3
      cv_out(CO1, cv_current_);
      cv_out(CO3, cv_current_ / 2.0);
    }
    else if (gate_)
    {
      // set the gate time to the time when the attack curve would have started for the current CV output value, so that
      // subsequent curve calculations are correct for retrigger (this will just be the current time if current CV is 0)
      gate_time_ = millis()
          - (unsigned long)(-float(adsr_values_[env_].A) * log(1 - (cv_current_ / (ENV_PEAK * ENV_SATURATION))));
    }

    widget_loop();

    // display selected envelope
    if (selected_env_ != last_selected_env_)
    {
      last_selected_env_ = selected_env_;
      Display.fill_rect(53, 55, 7, 9, DUINO_SH1106::Inverse);
      Display.fill_rect(68, 55, 7, 9, DUINO_SH1106::Inverse);
      Display.display(53, 74, 6, 7);
    }

    // display gate
    if (gate_ != last_gate_)
    {
      last_gate_ = gate_;
      if (gate_)
      {
        Display.draw_bitmap_7(60, 25, icons, 0, DUINO_SH1106::White);
      }
      else
      {
        Display.fill_rect(60, 25, 7, 7, DUINO_SH1106::Black);
      }
      Display.display(60, 66, 3, 3);
    }
  }

  void gate_callback()
  {
    gate_ = gt_read_debounce(DUINO_Function::GT3);
    if (gate_)
    {
      retrigger_ = true;
    }
  }

  void switch_callback()
  {
    if (millis() - debounce_ > DEBOUNCE_MS)
    {
      selected_env_++;
      selected_env_ %= 2;
      debounce_ = millis();
    }
  }

  void widget_adsr_scroll_callback(uint8_t selected, int delta)
  {
    const int8_t v_last = widget_save_->params.vals.v[selected];
    if (adjust<int8_t>(widget_save_->params.vals.v[selected], delta, 0, V_MAX))
    {
      // update slider
      Display.fill_rect(widgets_adsr_[selected]->x() - 1, 51 - v_last, 9, 3, DUINO_SH1106::Black);
      Display.fill_rect(widgets_adsr_[selected]->x() - 1, 51 - widget_save_->params.vals.v[selected], 9, 3,
          DUINO_SH1106::White);
      Display.display(widgets_adsr_[selected]->x() - 1, widgets_adsr_[selected]->x() + 7, 1, 6);

      // update ADSR value
      uint8_t e = selected > 3 ? 1 : 0;
      switch (selected % 4)
      {
        case 0:
          adsr_values_[e].A = uint16_t(widget_save_->params.vals.v[selected]) * 24;
          break;
        case 1:
          adsr_values_[e].D = uint16_t(widget_save_->params.vals.v[selected]) * 24;
          break;
        case 2:
          adsr_values_[e].S = (float(widget_save_->params.vals.v[selected]) / float(V_MAX)) * ENV_PEAK;
          break;
        case 3:
          adsr_values_[e].R = uint16_t(widget_save_->params.vals.v[selected]) * 24;
          break;
      }

      widget_save_->mark_changed();
      widget_save_->display();
    }
  }

private:
  struct ParameterValues
  {
    int8_t v[8];
  };

  DUINO_WidgetContainer<2> * container_outer_;
  DUINO_WidgetContainer<8> * container_adsr_;
  DUINO_SaveWidget<ParameterValues> * widget_save_;
  DUINO_DisplayWidget * widgets_adsr_[8];

  volatile bool gate_, retrigger_;
  volatile uint8_t selected_env_;
  volatile unsigned long debounce_;
  DU_ADSR_Values adsr_values_[2];
  uint8_t env_;
  unsigned long gate_time_, release_time_;
  float cv_current_, cv_released_;
  uint8_t last_selected_env_;
  bool last_gate_;
};

DU_ADSR_Function * function;

void gate_isr() { function->gate_callback(); }
void switch_isr() { function->switch_callback(); }

void adsr_scroll_callback(uint8_t selected, int delta) { function->widget_adsr_scroll_callback(selected, delta); }

void setup()
{
  function = new DU_ADSR_Function();

  function->begin();
}

void loop()
{
  function->function_loop();
}
