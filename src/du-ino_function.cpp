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
 * DU-INO Arduino Library - Function Module
 * Aaron Mavrinac <aaron@logick.ca>
 */

#include "du-ino_mcp4922.h"
#include "du-ino_widgets.h"
#include "du-ino_function.h"

#if __has_include("du-ino_calibration.h")
#include "du-ino_calibration.h"
#define USE_CALIBRATION
#endif

#define TRIG_MS           5   // ms
#define DIGITAL_THRESH    3.0 // V
#define CV_IN_OFFSET      0.1 // V

DUINO_Function::DUINO_Function(uint8_t sc)
  : top_level_widget_(NULL)
  , saved_(false)
{
  set_switch_config(sc);

  // configure analog pins
  analogReference(EXTERNAL);
  pinMode(A0, INPUT);
  pinMode(A1, INPUT);
  pinMode(A2, INPUT);
  pinMode(A3, INPUT);

  // configure DACs
  for (uint8_t i = 0; i < 2; ++i)
  {
    dac_[i] = new DUINO_MCP4922(7 - i, 8);
  }
}

void DUINO_Function::begin()
{
  static bool initialized = false;

  if (!initialized)
  {
    // initialize DACs
    dac_[0]->begin();
    dac_[1]->begin();

    // initialize outputs
    gt_out_multi(0xFF, false);

    // initialize display
    Display.begin();
    Display.clear_display(); 
    Display.display();

    // initialize encoder
    Encoder.begin();

    function_setup();
    initialized = true;
  }
}

void DUINO_Function::widget_setup(DUINO_Widget * top)
{
  if (!top)
  {
    return;
  }

  top_level_widget_ = top;
  top_level_widget_->invert(false);
}

void DUINO_Function::widget_loop()
{
  if (!top_level_widget_)
  {
    return;
  }

  // handle encoder button press
  DUINO_Encoder::Button b = Encoder.get_button();
  if(b == DUINO_Encoder::Clicked)
  {
    top_level_widget_->on_click();
  }
  else if(b == DUINO_Encoder::DoubleClicked)
  {
    top_level_widget_->on_double_click();
  }

  int16_t v = Encoder.get_value();
  if(v)
  {
    top_level_widget_->on_scroll(v);
  }
}

bool DUINO_Function::gt_read(DUINO_Function::Jack jack)
{
  switch (jack)
  {
    case GT1:
    case GT2:
    case GT3:
    case GT4:
      if (switch_config_ & (1 << jack))
      {
        return digitalRead(jack) == LOW ? true : false;
      }
      break;
    case CI1:
    case CI2:
    case CI3:
    case CI4:
      return cv_read(jack) > DIGITAL_THRESH;
  }

  return false;
}

bool DUINO_Function::gt_read_debounce(DUINO_Function::Jack jack)
{
  if (switch_config_ & (1 << jack))
  {
    uint16_t buffer = 0x5555;
    while (buffer && buffer != 0xFFFF)
    {
      buffer = (buffer << 1) | digitalRead(jack);
    }
    return buffer ? false : true;
  }
  else
  {
    return false;
  }
}

void DUINO_Function::gt_out(DUINO_Function::Jack jack, bool on, bool trig)
{
  switch (jack)
  {
    case GT1:
    case GT2:
    case GT3:
    case GT4:
      if ((~switch_config_) & (1 << jack))
      {
        digitalWrite(jack, on ? HIGH : LOW);
        if (trig)
        {
          delay(TRIG_MS);
          digitalWrite(jack, on ? LOW : HIGH);
        }
      }
      break;
    case CO1:
    case CO2:
    case CO3:
    case CO4:
      dac_[(jack - 4) >> 1]->output((DUINO_MCP4922::Channel)((jack - 4) & 1), on ? 0xBFF : 0x800);
      if (trig)
      {
        delay(TRIG_MS);
        dac_[(jack - 4) >> 1]->output((DUINO_MCP4922::Channel)((jack - 4) & 1), on ? 0x800 : 0xBFF);
      }
      break;
  }
}

void DUINO_Function::gt_out_multi(uint8_t jacks, bool on, bool trig)
{
  for (uint8_t i = 0; i < 4; ++i)
  {
    if (jacks & (~switch_config_) & (1 << i))
    {
      digitalWrite(i, on ? HIGH : LOW);
    }
  }
  for (uint8_t i = 4; i < 8; ++i)
  {
    if (jacks & (1 << i))
    {
      dac_[(i - 4) >> 1]->output((DUINO_MCP4922::Channel)((i - 4) & 1), on ? 0xBFF : 0x800);
    }
  }

  if (trig)
  {
    delay(TRIG_MS);
    for (uint8_t i = 0; i < 4; ++i)
    {
      if (jacks & (~switch_config_) & (1 << i))
      {
        digitalWrite(i, on ? LOW : HIGH);
      }
    }
    for (uint8_t i = 4; i < 8; ++i)
    {
      if (jacks & (1 << i))
      {
        dac_[(i - 4) >> 1]->output((DUINO_MCP4922::Channel)((i - 4) & 1), on ? 0x800 : 0xBFF);
      }
    }
  }
}

float DUINO_Function::cv_read(DUINO_Function::Jack jack)
{
  switch (jack)
  {
#ifdef USE_CALIBRATION
    case CI1:
      return cv_analog_read(A0) * CI1_PRESCALE + CI1_OFFSET;
    case CI2:
      return cv_analog_read(A1) * CI2_PRESCALE + CI2_OFFSET;
    case CI3:
      return cv_analog_read(A2) * CI3_PRESCALE + CI3_OFFSET;
    case CI4:
      return cv_analog_read(A3) * CI4_PRESCALE + CI4_OFFSET;
#else
    case CI1:
      return cv_analog_read(A0);
    case CI2:
      return cv_analog_read(A1);
    case CI3:
      return cv_analog_read(A2);
    case CI4:
      return cv_analog_read(A3);
#endif
    default:
      return 0.0;
  }
}

void DUINO_Function::cv_out(DUINO_Function::Jack jack, float value)
{
  if (jack == CO1 || jack == CO2 || jack == CO3 || jack == CO4)
  {
    // (value + 10) * ((2^12 - 1) / 20)
#ifdef USE_CALIBRATION
    float calibrated_value;
    switch (jack)
    {
      case CO1:
        calibrated_value = value * CO1_PRESCALE + CO1_OFFSET;
      case CO2:
        calibrated_value = value * CO2_PRESCALE + CO2_OFFSET;
      case CO3:
        calibrated_value = value * CO3_PRESCALE + CO3_OFFSET;
      case CO4:
        calibrated_value = value * CO4_PRESCALE + CO4_OFFSET;
    }
#else
    const float calibrated_value = value;
#endif
    uint16_t data = uint16_t((calibrated_value + 10.0) * 204.75);

    // DAC output
    dac_[(jack - 4) >> 1]->output((DUINO_MCP4922::Channel)((jack - 4) & 1), data);
  }
}

void DUINO_Function::cv_hold(bool state)
{
  // both DACs share the LDAC pin, so holding either will hold all four channels
  dac_[0]->hold(state);
}

void DUINO_Function::gt_attach_interrupt(DUINO_Function::Jack jack, void (*isr)(void), int mode)
{
  if (jack == GT3 || jack == GT4)
  {
    attachInterrupt(digitalPinToInterrupt(jack), isr, mode);
  }
}

void DUINO_Function::gt_detach_interrupt(DUINO_Function::Jack jack)
{
  if (jack == GT3 || jack == GT4)
  {
    detachInterrupt(digitalPinToInterrupt(jack));
  }
}

void DUINO_Function::set_switch_config(uint8_t sc)
{
  switch_config_ = sc;

  // configure digital pins
  for (uint8_t i = 0; i < 4; ++i)
  {
    pinMode(i, sc & (1 << i) ? INPUT : OUTPUT);
  }
}

float DUINO_Function::cv_analog_read(uint8_t pin)
{
  // value * (20 / (2^10 - 1)) - 10
  return float(analogRead(pin)) * 0.019550342130987292 - 10.0 + CV_IN_OFFSET;
}
