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
#include <TimerOne.h>

struct DU_ADSR_Values {
  uint16_t A;  // ms
  uint16_t D;  // ms
  float S;    // V
  uint16_t R;  // ms
};

volatile DU_ADSR_Values adsr_values;
volatile bool gate;

const unsigned char label[] PROGMEM = {'A', 'D', 'S', 'R'};

void gate_isr();
void timer_isr();

class DU_ADSR_Function : public DUINO_Function {
 public:
  DU_ADSR_Function() : DUINO_Function(0b0000000100) { }
  
  virtual void setup()
  {
    gate_time = 0;
    gt_attach_interrupt(GT3, gate_isr, CHANGE);
  }

  virtual void loop()
  {
    if(gate_time)
    {
      if(gate)
      {
        uint16_t elapsed = millis() - gate_time;
        if(elapsed < adsr_values.A)
        {
          // attack
          cv_current = (float(elapsed) / float(adsr_values.A)) * 10.0;
        }
        else if(elapsed < adsr_values.A + adsr_values.D)
        {
          // decay
          cv_current = adsr_values.S + (1.0 - float(elapsed - adsr_values.A) / float(adsr_values.D))
                       * (10.0 - adsr_values.S);
        }
        else
        {
          // sustain
          cv_current = adsr_values.S;
        }
      }
      else
      {
        if(release_time)
        {
          uint16_t elapsed = millis() - release_time;
          if(elapsed < adsr_values.R)
          {
            // release
            cv_current = (1.0 - float(elapsed) / float(adsr_values.R)) * cv_released;
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
      gate_time = millis();
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

    // initialize ADSR values
    v[0] = v_last[0] = 5;
    v[1] = v_last[1] = 10;
    v[2] = v_last[2] = 15;
    v[3] = v_last[3] = 5; 
    adsr_values.A = uint16_t(v[0]) * 30;
    adsr_values.D = uint16_t(v[1]) * 30;
    adsr_values.S = (float(v[2]) / 32.0) * 10.0;
    adsr_values.R = uint16_t(v[3]) * 30;

    // draw top line
    display->draw_du_logo_sm(1, 1, DUINO_SSD1306::White);
    display->draw_text(41, 10, "ADSR/VCA", DUINO_SSD1306::White);
    display->draw_text(100, 10, "GATE", DUINO_SSD1306::White);

    // draw sliders
    for(uint8_t i = 0; i < 4; ++i)
    {
      display->fill_rect(32 * selected + 11, 54 - v[selected], 9, 3, DUINO_SSD1306::White);
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

  virtual void timer()
  {
    // handle encoder button press
    DUINO_Encoder::Button b = encoder->get_button();
    if(b == DUINO_Encoder::Clicked)
    {
      display->fill_rect(32 * selected + 11, 55, 9, 9, DUINO_SSD1306::Black);
      display->draw_char(32 * selected + 13, 56, label[selected], DUINO_SSD1306::White);
      selected++;
      selected %= 4;
      display->fill_rect(32 * selected + 11, 55, 9, 9, DUINO_SSD1306::White);
      display->draw_char(32 * selected + 13, 56, label[selected], DUINO_SSD1306::Black);
    }

    // handle encoder spin
    v[selected] += encoder->get_value();
    if(v[selected] < 0)
      v[selected] = 0;
    if(v[selected] > 32)
      v[selected] = 32;
    if(v[selected] != v_last[selected])
    {
      // update slider
      display->fill_rect(32 * selected + 11, 54 - v_last[selected], 9, 3, DUINO_SSD1306::Black);
      display->fill_rect(32 * selected + 11, 54 - v[selected], 9, 3, DUINO_SSD1306::White);

      // update ADSR value
      switch(selected)
      {
        case 0:
          adsr_values.A = uint16_t(v[selected]) * 30;
          break;
        case 1:
          adsr_values.D = uint16_t(v[selected]) * 30;
          break;
        case 2:
          adsr_values.S = (float(v[selected]) / 32.0) * 10.0;
          break;
        case 3:
          adsr_values.R = uint16_t(v[selected]) * 30;
          break;
      }
      // update last encoder value
      v_last[selected] = v[selected];
    }

    // display gate state
    display->fill_rect(98, 8, 28, 9, gate ? DUINO_SSD1306::White : DUINO_SSD1306::Black);
    display->draw_text(100, 10, "GATE", DUINO_SSD1306::Inverse);

    display->display();
  }

 private:
  void display_gate()
  {

  }

  uint8_t selected;
  int8_t v[4], v_last[4];
};

DU_ADSR_Function * function;
DU_ADSR_Interface * interface;

void gate_isr()
{
  gate = function->gt_read(GT3);
}

void timer_isr()
{
  interface->timer_isr();
}

void setup() {
  gate = 0;

  function = new DU_ADSR_Function();
  interface = new DU_ADSR_Interface();

  function->begin();
  interface->begin();

  Timer1.initialize(1000);
  Timer1.attachInterrupt(timer_isr);
}

void loop() {
  function->loop();
}
