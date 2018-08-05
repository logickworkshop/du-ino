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
 */

#include <du-ino_function.h>
#include <du-ino_interface.h>

#define ENV_PEAK              10.0 // V
#define ENV_SATURATION        1.581976
#define ENV_DECAY_COEFF       -2.0
#define ENV_RELEASE_COEFF     -2.0
#define ENV_RELEASE_HOLD      3

#define V_MAX                 43

#define DEBOUNCE_MS           100 // ms

static const unsigned char icons[] PROGMEM = {
  0x38, 0x78, 0x7f, 0x7f, 0x7f, 0x78, 0x38  // gate
};

struct DU_ADSR_Values {
  uint16_t A;  // ms
  uint16_t D;  // ms
  float S;     // V
  uint16_t R;  // ms
};

DU_ADSR_Values adsr_values[2];
volatile bool gate, retrigger;
volatile uint8_t selected_env;
volatile unsigned long debounce;

static const unsigned char label[4] = {'A', 'D', 'S', 'R'};

void gate_isr();
void switch_isr();

class DU_ADSR_Function : public DUINO_Function {
 public:
  DU_ADSR_Function() : DUINO_Function(0b10111100) { }
  
  virtual void setup()
  {
    env = 0;
    gate_time = 0;
    release_time = 0;

    gt_attach_interrupt(GT3, gate_isr, CHANGE);
    gt_attach_interrupt(GT4, switch_isr, FALLING);
  }

  virtual void loop()
  {
    if(retrigger)
    {
      env = selected_env;
      gate_time = 0;
      release_time = 0;
      retrigger = false;
    }

    if(gate_time)
    {
      if(gate)
      {
        uint16_t elapsed = millis() - gate_time;
        if(elapsed < adsr_values[env].A)
        {
          // attack
          cv_current = ENV_PEAK * ENV_SATURATION * (1.0 - exp(-(float(elapsed) / float(adsr_values[env].A))));
        }
        else
        {
          // decay/sustain
          cv_current = adsr_values[env].S + (ENV_PEAK - adsr_values[env].S)
              * exp(ENV_DECAY_COEFF * (float(elapsed - adsr_values[env].A) / float(adsr_values[env].D)));
        }
      }
      else
      {
        if(release_time)
        {
          uint16_t elapsed = millis() - release_time;
          if(elapsed < adsr_values[env].R * ENV_RELEASE_HOLD)
          {
            // release
            cv_current = exp(ENV_RELEASE_COEFF * (float(elapsed) / float(adsr_values[env].R))) * cv_released;
          }
          else
          {
            cv_current = 0.0;
            release_time = 0;
            gate_time = 0;
          }
        }
        else
        {
          release_time = millis();
          cv_released = cv_current;
        }
      }

      cv_out(CO1, cv_current);
      cv_out(CO3, cv_current / 2.0);
    }
    else if(gate)
    {
      gate_time = millis()
          - (unsigned long)(-float(adsr_values[env].A) * log(1 - (cv_current / (ENV_PEAK * ENV_SATURATION))));
    }
  }

 private:
  uint8_t env;
  unsigned long gate_time;
  unsigned long release_time;
  float cv_current;
  float cv_released;
};

class DU_ADSR_Interface : public DUINO_Interface {
 public:
  virtual void setup()
  {
    // initialize interface
    selected = 0;
    saving = false;
    last_selected_env = 0;
    last_gate = false;

    // load/initialize ADSR values
    load_params(0, (uint8_t *)v, 8);
    for(uint8_t i = 0; i < 8; ++i)
    {
      if(v[i] < 0)
      {
        v[i] = 0;
      }
      else if(v[i] > V_MAX)
      {
        v[i] = V_MAX;
      }
      v_last[i] = v[i];
    }
    for(uint8_t e = 0; e < 2; ++e)
    {
      adsr_values[e].A = uint16_t(v[2 * e]) * 24;
      adsr_values[e].D = uint16_t(v[2 * e + 1]) * 24;
      adsr_values[e].S = (float(v[2 * e + 2]) / float(V_MAX)) * ENV_PEAK;
      adsr_values[e].R = uint16_t(v[2 * e + 3]) * 24;
    }

    // draw title
    display->draw_du_logo_sm(0, 0, DUINO_SH1106::White);
    display->draw_text(16, 0, "ADSR/VCA", DUINO_SH1106::White);

    // draw save box
    display->fill_rect(122, 1, 5, 5, DUINO_SH1106::White);

    // draw envelope indicators
    display->draw_char(47, 56, 0x11, DUINO_SH1106::White);
    display->draw_char(54, 56, '1', DUINO_SH1106::White);
    display->draw_char(69, 56, '2', DUINO_SH1106::White);
    display->draw_char(76, 56, 0x10, DUINO_SH1106::White);

    // draw sliders & labels
    for(uint8_t i = 0; i < 8; ++i)
    {
      display->fill_rect(11 * (i % 4) + (i > 3 ? 85 : 1), 51 - v[i], 9, 3, DUINO_SH1106::White);
      display->draw_char(11 * (i % 4) + (i > 3 ? 87 : 3), 56, label[i % 4], DUINO_SH1106::White);
    }

    display->fill_rect(1, 55, 9, 9, DUINO_SH1106::Inverse);
    display->fill_rect(53, 55, 7, 9, DUINO_SH1106::Inverse);

    display->display_all();
  }

  virtual void loop()
  {
    // handle encoder button press
    DUINO_Encoder::Button b = encoder->get_button();
    if(b == DUINO_Encoder::DoubleClicked)
    {
      invert_current_selection();
      saving = !saving;
      invert_current_selection();
    }
    else if(b == DUINO_Encoder::Clicked)
    {
      if(saving)
      {
        if(!saved)
        {
          save_params(0, v, 8);
          display->fill_rect(123, 2, 3, 3, DUINO_SH1106::Black);
          display->display(123, 125, 0, 0);
        }
      }
      else
      {
        invert_current_selection();
        selected++;
        selected %= 8;
        invert_current_selection();
      }
    }

    // handle encoder spin
    if(!saving)
    {
      v[selected] += encoder->get_value();
      if(v[selected] < 0)
      {
        v[selected] = 0;
      }
      if(v[selected] > V_MAX)
      {
        v[selected] = V_MAX;
      }
      if(v[selected] != v_last[selected])
      {
        // mark save box
        if(saved)
        {
          saved = false;
          display->fill_rect(123, 2, 3, 3, DUINO_SH1106::Black);
          display->display(123, 125, 0, 0);
        }

        // update slider
        uint16_t col = 11 * (selected % 4) + (selected > 3 ? 85 : 1);
        display->fill_rect(col, 51 - v_last[selected], 9, 3, DUINO_SH1106::Black);
        display->fill_rect(col, 51 - v[selected], 9, 3, DUINO_SH1106::White);
        display->display(col, col + 8, 1, 6);

        // update ADSR value
        uint8_t e = selected > 3 ? 1 : 0;
        switch(selected % 4)
        {
          case 0:
            adsr_values[e].A = uint16_t(v[selected]) * 24;
            break;
          case 1:
            adsr_values[e].D = uint16_t(v[selected]) * 24;
            break;
          case 2:
            adsr_values[e].S = (float(v[selected]) / float(V_MAX)) * ENV_PEAK;
            break;
          case 3:
            adsr_values[e].R = uint16_t(v[selected]) * 24;
            break;
        }

        // update last encoder value
        v_last[selected] = v[selected];
      }
    }

    // display selected envelope
    if(selected_env != last_selected_env)
    {
      last_selected_env = selected_env;
      display->fill_rect(53, 55, 7, 9, DUINO_SH1106::Inverse);
      display->fill_rect(68, 55, 7, 9, DUINO_SH1106::Inverse);
      display->display(53, 74, 6, 7);
    }

    // display gate
    if(gate != last_gate)
    {
      last_gate = gate;
      if(gate)
      {
        display->draw_bitmap_7(60, 25, icons, 0, DUINO_SH1106::White);
      }
      else
      {
        display->fill_rect(60, 25, 7, 7, DUINO_SH1106::Black);
      }
      display->display(60, 66, 3, 3);
    }
  }

 private:
  void invert_current_selection()
  {
    if(saving)
    {
      display->fill_rect(121, 0, 7, 7, DUINO_SH1106::Inverse);
      display->display(121, 127, 0, 0);
    }
    else
    {
      display->fill_rect(11 * (selected % 4) + (selected > 3 ? 85 : 1), 55, 9, 9, DUINO_SH1106::Inverse);
      display->display(11 * (selected % 4) + (selected > 3 ? 85 : 1), 11 * (selected % 4) + (selected > 3 ? 93 : 9),
          6, 7);
    }
    delay(1);  // FIXME: SH1106 unstable without this (too many I2C txs too quickly?)
  }

  uint8_t selected;
  bool saving;
  int8_t v[8], v_last[8];
  uint8_t last_selected_env;
  bool last_gate;
};

DU_ADSR_Function * function;
DU_ADSR_Interface * interface;

ENCODER_ISR(interface->encoder);

void gate_isr()
{
  gate = function->gt_read_debounce(GT3);
  if(gate)
    retrigger = true;
}

void switch_isr()
{
  if(millis() - debounce > DEBOUNCE_MS)
  {
    selected_env++;
    selected_env %= 2;
    debounce = millis();
  }
}

void setup()
{
  gate = retrigger = false;
  selected_env = 0;
  debounce = 0;

  function = new DU_ADSR_Function();
  interface = new DU_ADSR_Interface();

  function->begin();
  interface->begin();
}

void loop()
{
  function->loop();
  interface->loop();
}
