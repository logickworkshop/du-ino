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
#include "du-ino_function.h"

#define TRIG_MS     5

DUINO_Function::DUINO_Function(uint16_t sc)
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
    dac[i] = new DUINO_MCP4922(i + 6, 8);
}

void DUINO_Function::begin()
{
  static bool initialized = false;

  if(!initialized)
  {
    dac[0]->begin();
    dac[1]->begin();

    dac[0]->output(DUINO_MCP4922::A, 0x800);
    dac[0]->output(DUINO_MCP4922::B, 0x800);
    dac[1]->output(DUINO_MCP4922::A, 0x800);
    dac[1]->output(DUINO_MCP4922::B, 0x800);

    gt_out(GT_ALL, false);

    setup();
    initialized = true;
  }
}

bool DUINO_Function::gt_read(uint8_t jack)
{
  if(jack < 4 && switch_config & (1 << jack))
  {
    return digitalRead(jack) == LOW ? true : false;
  }
  else
  { 
    return false;
  }
}

bool DUINO_Function::gt_read_debounce(uint8_t jack)
{
  if(jack < 4 && switch_config & (1 << jack))
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

void DUINO_Function::gt_out(uint8_t jack, bool on, bool trig)
{
  if(!(jack & GT_MULTI))
  {
    if(jack < 6 && out_mask & (1 << jack))
    {
      digitalWrite(jack, on ? HIGH : LOW);
      if(trig)
      {
        delay(TRIG_MS);
        digitalWrite(jack, on ? LOW : HIGH);
      }
    }
  }
  else
  {
    for(uint8_t i = 0; i < 6; ++i)
      if(jack & out_mask & (1 << i))
        digitalWrite(i, on ? HIGH : LOW);
    if(trig)
    {
      delay(TRIG_MS);
      for(uint8_t i = 0; i < 6; ++i)
        if(jack & out_mask & (1 << i))
          digitalWrite(i, on ? LOW : HIGH);
    }
  }
}

float DUINO_Function::cv_read(uint8_t jack)
{
  // value * (20 / (2^10 - 1)) - 10
  return float(analogRead(jack)) * 0.019550342130987292 - 10.0;
}

void DUINO_Function::cv_out(uint8_t jack, float value)
{
  // (value + 10) * ((2^12 - 1) / 20)
  uint16_t data = uint16_t((value + 10.0) * 204.75);

  // DAC output
  dac[jack >> 1]->output((DUINO_MCP4922::Channel)(jack & 1), data);
}

void DUINO_Function::cv_hold(bool state)
{
  // both DACs share the LDAC pin, so holding either will hold all four channels
  dac[0]->hold(state);
}

void DUINO_Function::gt_attach_interrupt(uint8_t jack, void (*isr)(void), int mode)
{
  attachInterrupt(digitalPinToInterrupt(jack), isr, mode);
}

void DUINO_Function::gt_detach_interrupt(uint8_t jack)
{
  detachInterrupt(digitalPinToInterrupt(jack));
}

void DUINO_Function::set_switch_config(uint16_t sc)
{
  switch_config = sc;
  out_mask = 0x30 | (~sc & 0x0F);

  // configure digital pins
  for(uint8_t i = 0; i < 6; ++i)
    pinMode(i, out_mask & (1 << i) ? OUTPUT : INPUT);
}
