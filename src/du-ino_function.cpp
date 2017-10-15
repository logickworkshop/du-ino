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
  : switch_config(sc)
  , out_mask(0x30 || (~sc & 0x0F))
{
  // configure analog pins
  analogReference(EXTERNAL);
  pinMode(A0, INPUT);
  pinMode(A1, INPUT);
  pinMode(A2, INPUT);
  pinMode(A3, INPUT);
  
  // configure digital pins
  pinMode(5, OUTPUT);
  pinMode(4, OUTPUT);
  for(uint8_t i = 0; i < 4; ++i)
    pinMode(i, switch_config & (1 << i) ? INPUT : OUTPUT);

  // configure DACs
  for(uint8_t i = 0; i < 2; ++i)
    dac[i] = new DUINO_MCP4922(i + 6);
}

void DUINO_Function::begin()
{
  static bool initialized = false;

  if(!initialized)
  {
    gt_out(GT_ALL, false);
    cv_out(CO1, 0.0);
    cv_out(CO2, 0.0);
    cv_out(CO3, 0.0);
    cv_out(CO4, 0.0);

    initialized = true;
  }
}

void DUINO_Function::run()
{
  unsigned long time_current = millis();
  unsigned long dt = time_current - time_last;
  time_last = time_current;

  loop(dt);
}

bool DUINO_Function::gt_read(uint8_t jack)
{
  if(jack < 4 && switch_config & (1 << jack))
    return (bool)digitalRead(jack);
  else
    return false;
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
        digitalWrite(jack , on ? LOW : HIGH);
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
  uint16_t data = (value + 10.0) * 204.75;

  // DAC output
  dac[jack >> 1]->output((DUINO_MCP4922::Channel)(jack & 1), data);
}

void gt_attach_interrupt(uint8_t jack, void (*isr)(void), int mode)
{
  attachInterrupt(digitalPinToInterrupt(jack), isr, mode);
}

void gt_detach_interrupt(uint8_t jack)
{
  detachInterrupt(digitalPinToInterrupt(jack));
}
