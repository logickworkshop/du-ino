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

#define ENV_PEAK              5.0
#define ENV_SATURATION        1.581976
#define ENV_DECAY_COEFF       -2.0
#define ENV_RELEASE_COEFF     -2.0
#define ENV_RELEASE_HOLD      3

#define V_MAX                 43

struct DU_ADSR_Values {
  uint16_t A;  // ms
  uint16_t D;  // ms
  float S;     // V
  uint16_t R;  // ms
};

DU_ADSR_Values adsr_values;
volatile bool gate, retrigger;

static const unsigned char label[4] = {'A', 'D', 'S', 'R'};

void gate_isr();

class DU_ADSR_Function : public DUINO_Function {
 public:
  DU_ADSR_Function() : DUINO_Function(0b0000000100) { }
  
  virtual void setup()
  {
    gate_time = 0;
    release_time = 0;
    retrigger = false;
    gt_attach_interrupt(GT3, gate_isr, CHANGE);
  }

  virtual void loop()
  {
    if(retrigger)
    {
      gate_time = 0;
      release_time = 0;
      retrigger = false;
    }

    if(gate_time)
    {
      if(gate)
      {
        uint16_t elapsed = millis() - gate_time;
        if(elapsed < adsr_values.A)
        {
          // attack
          cv_current = ENV_PEAK * ENV_SATURATION * (1.0 - exp(-(float(elapsed) / float(adsr_values.A))));
        }
        else
        {
          // decay/sustain
          cv_current = adsr_values.S + (ENV_PEAK - adsr_values.S)
              * exp(ENV_DECAY_COEFF * (float(elapsed - adsr_values.A) / float(adsr_values.D)));
        }
      }
      else
      {
        if(release_time)
        {
          uint16_t elapsed = millis() - release_time;
          if(elapsed < adsr_values.R * ENV_RELEASE_HOLD)
          {
            // release
            cv_current = exp(ENV_RELEASE_COEFF * (float(elapsed) / float(adsr_values.R))) * cv_released;
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
    }
    else if(gate)
    {
      gate_time = millis()
          - (unsigned long)(-float(adsr_values.A) * log(1 - (cv_current / (ENV_PEAK * ENV_SATURATION))));
    }
  }

 private:
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
    display_changed = false;

    // load/initialize ADSR values
    load_params(0, (uint8_t *)v, 4);
    for(uint8_t i = 0; i < 4; ++i)
    {
      if(v[i] < 0)
      {
        v[i] = 0;
      }
      else if(v[i] > V_MAX)
      {
        v[i] = V_MAX;
      }
    }
    v_last[0] = v[0];
    v_last[1] = v[1];
    v_last[2] = v[2];
    v_last[3] = v[3]; 
    adsr_values.A = uint16_t(v[0]) * 24;
    adsr_values.D = uint16_t(v[1]) * 24;
    adsr_values.S = (float(v[2]) / float(V_MAX)) * ENV_PEAK;
    adsr_values.R = uint16_t(v[3]) * 24;

    // draw title
    display->draw_du_logo_sm(0, 0, DUINO_SSD1306::White);
    display->draw_text(16, 0, "ADSR/VCA", DUINO_SSD1306::White);

    // draw save box
    display->fill_rect(122, 1, 5, 5, DUINO_SSD1306::White);

    // draw sliders
    for(uint8_t i = 0; i < 4; ++i)
    {
      display->fill_rect(32 * i + 11, 51 - v[i], 9, 3, DUINO_SSD1306::White);
    }

    // draw labels
    display->fill_rect(11, 55, 9, 9, DUINO_SSD1306::White);
    display->draw_char(13, 56, label[0], DUINO_SSD1306::Black);
    for(uint8_t i = 1; i < 4; ++i)
    {
      display->draw_char(32 * i + 13, 56, label[i], DUINO_SSD1306::White);
    }

    display->display();
  }

  virtual void loop()
  {
    // handle encoder button press
    DUINO_Encoder::Button b = encoder->get_button();
    if(b == DUINO_Encoder::DoubleClicked)
    {
      saving = !saving;
      display->fill_rect(121, 0, 7, 7, DUINO_SSD1306::Inverse);
      display->fill_rect(32 * selected + 11, 55, 9, 9, DUINO_SSD1306::Inverse);
      display_changed = true;
    }
    else if(b == DUINO_Encoder::Clicked)
    {
      if(saving)
      {
        save_params(0, v, 4);
        display->fill_rect(123, 2, 3, 3, DUINO_SSD1306::Black);
        display_changed = true;
      }
      else
      {
        display->fill_rect(32 * selected + 11, 55, 9, 9, DUINO_SSD1306::Inverse);
        selected++;
        selected %= 4;
        display->fill_rect(32 * selected + 11, 55, 9, 9, DUINO_SSD1306::Inverse);
        display_changed = true;
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
        display->fill_rect(123, 2, 3, 3, DUINO_SSD1306::Black);

        // update slider
        display->fill_rect(32 * selected + 11, 51 - v_last[selected], 9, 3, DUINO_SSD1306::Black);
        display->fill_rect(32 * selected + 11, 51 - v[selected], 9, 3, DUINO_SSD1306::White);
        display_changed = true;

        // update ADSR value
        switch(selected)
        {
          case 0:
            adsr_values.A = uint16_t(v[selected]) * 24;
            break;
          case 1:
            adsr_values.D = uint16_t(v[selected]) * 24;
            break;
          case 2:
            adsr_values.S = (float(v[selected]) / float(V_MAX)) * ENV_PEAK;
            break;
          case 3:
            adsr_values.R = uint16_t(v[selected]) * 24;
            break;
        }

        // update last encoder value
        v_last[selected] = v[selected];
      }
    }

    if(display_changed)
    {
      display->display();
      display_changed = false;
    }
  }

 private:
  uint8_t selected;
  int8_t v[4], v_last[4];
  bool display_changed;
  bool saving;
};

DU_ADSR_Function * function;
DU_ADSR_Interface * interface;

ENCODER_ISR(interface->encoder);

void gate_isr()
{
  gate = function->gt_read(GT3);
  if(gate)
    retrigger = true;
}

void setup()
{
  gate = 0;

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
