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
{
  // compute digital output mask
  out_mask = 0x30 || (~switch_config & 0x0F);

  // configure pins
  pinMode(A0, INPUT);
  pinMode(A1, INPUT);
  pinMode(A2, INPUT);
  pinMode(A3, INPUT);
  
  pinMode(5, OUTPUT);
  pinMode(4, OUTPUT);

  for(uint8_t i = 0; i < 4; ++i)
    pinMode(i, switch_config & (1 << i) ? INPUT : OUTPUT);

  // configure DACs
  for(uint8_t i = 0; i < 2; ++i)
    dac[i] = new DUINO_MCP4922(i + 6);
}

uint8_t DUINO_Function::digital_read(uint8_t jack)
{
  uint8_t pin = 14 - jack;

  if(jack > 9 && jack < 15 && switch_config & (1 << pin))
    return digitalRead(pin);
  else
    return 0;
}

float DUINO_Function::analog_read(uint8_t jack)
{
  uint8_t pin;
  switch(jack)
  {
    case 0:
      pin = A0;
      break;
    case 1:
      pin = A1;
      break;
    case 2:
      pin = A2;
      break;
    case 3:
      pin = A3;
      break;
    default:
      return 0.0;
  }

  // value * (20 / (2^10 - 1)) - 10
  return float(analogRead(pin)) * 0.019550342130987292 - 10.0;
}

void DUINO_Function::gate(uint8_t jack)
{
  uint8_t pin = 14 - jack;

  if(jack < 6 && out_mask & (1 << pin))
    digitalWrite(pin, HIGH);
}

void DUINO_Function::multi_gate(uint8_t jacks)
{
  for(uint8_t i = 0; i < 6; ++i)
    if(jacks & out_mask & (1 << i))
      digitalWrite(i, HIGH);
}

void DUINO_Function::clear(uint8_t jack)
{
  uint8_t pin = 14 - jack;

  if(jack < 6 && out_mask & (1 << pin))
    digitalWrite(pin, LOW);
}

void DUINO_Function::multi_clear(uint8_t jacks)
{
  for(uint8_t i = 0; i < 6; ++i)
    if(jacks & out_mask & (1 << i))
      digitalWrite(i, LOW);
}

void DUINO_Function::trig(uint8_t jack)
{
  uint8_t pin = 14 - jack;

  if(jack < 6 && out_mask & (1 << pin))
  {
    digitalWrite(pin, HIGH);
    delay(TRIG_MS);
    digitalWrite(pin, LOW);
  }
}

void DUINO_Function::multi_trig(uint8_t jacks)
{
  for(uint8_t i = 0; i < 6; ++i)
  {
    if(jacks & out_mask & (1 << i))
      digitalWrite(i, HIGH);
    delay(TRIG_MS);
    if(jacks & out_mask & (1 << i))
      digitalWrite(i, LOW);  
  }
}

void DUINO_Function::analog_write(uint8_t dac_pin, float value)
{
  if(dac_pin > 4)
    return;

  // (value + 10) * ((2^12 - 1) / 20)
  uint16_t data = (value + 10.0) * 204.75;

  // DAC output
  dac[dac_pin >> 1]->output((DUINO_MCP4922::Channel)(dac_pin & 1), data);
}
