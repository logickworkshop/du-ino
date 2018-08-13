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

#include <EEPROM.h>
#include "du-ino_mcp4922.h"
#include "du-ino_function.h"

#define TRIG_MS           5   // ms
#define DIGITAL_THRESH    3.0 // V

DUINO_Function::DUINO_Function(uint8_t sc)
  : display(new DUINO_SH1106(5, 4))
  , encoder(new DUINO_Encoder(9, 10, 12))
  , saved(false)
{
  set_switch_config(sc);

  // configure analog pins
  analogReference(EXTERNAL);
  pinMode(A0, INPUT);
  pinMode(A1, INPUT);
  pinMode(A2, INPUT);
  pinMode(A3, INPUT);

  // configure DACs
  for(uint8_t i = 0; i < 2; ++i)
    dac[i] = new DUINO_MCP4922(7 - i, 8);
}

void DUINO_Function::begin()
{
  static bool initialized = false;

  if(!initialized)
  {
    // initialize DACs
    dac[0]->begin();
    dac[1]->begin();

    // initialize outputs
    gt_out_multi(0xFF, false);

    // initialize display
    display->begin();
    display->clear_display(); 
    display->display_all();

    // initialize encoder
    encoder->begin();

    setup();
    initialized = true;
  }
}

bool DUINO_Function::gt_read(Jack jack)
{
  switch(jack)
  {
    case GT1:
    case GT2:
    case GT3:
    case GT4:
      if(switch_config & (1 << jack))
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

bool DUINO_Function::gt_read_debounce(Jack jack)
{
  if(switch_config & (1 << jack))
  {
    uint16_t buffer = 0x5555;
    while(buffer && buffer != 0xFFFF)
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

void DUINO_Function::gt_out(Jack jack, bool on, bool trig)
{
  switch(jack)
  {
    case GT1:
    case GT2:
    case GT3:
    case GT4:
      if((~switch_config) & (1 << jack))
      {
        digitalWrite(jack, on ? HIGH : LOW);
        if(trig)
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
      dac[(jack - 4) >> 1]->output((DUINO_MCP4922::Channel)((jack - 4) & 1), 0xBFF);
      if(trig)
      {
        delay(TRIG_MS);
        dac[(jack - 4) >> 1]->output((DUINO_MCP4922::Channel)((jack - 4) & 1), 0x800);
      }
      break;
  }
}

void DUINO_Function::gt_out_multi(uint8_t jacks, bool on, bool trig)
{
  for(uint8_t i = 0; i < 4; ++i)
    if(jacks & (~switch_config) & (1 << i))
      digitalWrite(i, on ? HIGH : LOW);
  for(uint8_t i = 4; i < 8; ++i)
    if(jacks & (1 << i))
      dac[(i - 4) >> 1]->output((DUINO_MCP4922::Channel)((i - 4) & 1), on ? 0xBFF : 0x800);

  if(trig)
  {
    delay(TRIG_MS);
    for(uint8_t i = 0; i < 4; ++i)
      if(jacks & (~switch_config) & (1 << i))
        digitalWrite(i, on ? LOW : HIGH);
    for(uint8_t i = 4; i < 8; ++i)
      if(jacks & (1 << i))
        dac[(i - 4) >> 1]->output((DUINO_MCP4922::Channel)((i - 4) & 1), on ? 0x800 : 0xBFF);
  }
}

float DUINO_Function::cv_read(Jack jack)
{
  switch(jack)
  {
    case CI1:
      return cv_analog_read(A0);
    case CI2:
      return cv_analog_read(A1);
    case CI3:
      return cv_analog_read(A2);
    case CI4:
      return cv_analog_read(A3);
    default:
      return 0.0;
  }
}

void DUINO_Function::cv_out(Jack jack, float value)
{
  if(jack == CO1 || jack == CO2 || jack == CO3 || jack == CO4)
  {
    // (value + 10) * ((2^12 - 1) / 20)
    uint16_t data = uint16_t((value + 10.0) * 204.75);

    // DAC output
    dac[(jack - 4) >> 1]->output((DUINO_MCP4922::Channel)((jack - 4) & 1), data);
  }
}

void DUINO_Function::cv_hold(bool state)
{
  // both DACs share the LDAC pin, so holding either will hold all four channels
  dac[0]->hold(state);
}

void DUINO_Function::gt_attach_interrupt(Jack jack, void (*isr)(void), int mode)
{
  if(jack == GT3 || jack == GT4)
  {
    attachInterrupt(digitalPinToInterrupt(jack), isr, mode);
  }
}

void DUINO_Function::gt_detach_interrupt(Jack jack)
{
  if(jack == GT3 || jack == GT4)
  {
    detachInterrupt(digitalPinToInterrupt(jack));
  }
}

void DUINO_Function::save_params(int address, uint8_t * start, int length)
{
  if(saved)
  {
    return;
  }

  for(int i = 0; i < length; ++i)
  {
    EEPROM.write(address + i, *(start + i));
  }

  saved = true;
}

void DUINO_Function::load_params(int address, uint8_t * start, int length)
{
  for(int i = 0; i < length; ++i)
  {
    *(start + i) = EEPROM.read(address + i);
  }

  saved = true;
}

void DUINO_Function::set_switch_config(uint8_t sc)
{
  switch_config = sc;

  // configure digital pins
  for(uint8_t i = 0; i < 4; ++i)
    pinMode(i, sc & (1 << i) ? INPUT : OUTPUT);
}

float DUINO_Function::cv_analog_read(uint8_t pin)
{
  // value * (20 / (2^10 - 1)) - 10
  return float(analogRead(pin)) * 0.019550342130987292 - 10.0;
}
