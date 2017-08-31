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

  return float(analogRead(pin)) * (20.0 / 1023.0) - 10;
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
    // TODO: delay
    digitalWrite(pin, LOW);
  }
}

void DUINO_Function::multi_trig(uint8_t jacks)
{
  for(uint8_t i = 0; i < 6; ++i)
  {
    if(jacks & out_mask & (1 << i))
      digitalWrite(i, HIGH);
    // TODO: delay
    if(jacks & out_mask & (1 << i))
      digitalWrite(i, LOW);  
  }
}

void DUINO_Function::analog_write(uint8_t dac_pin, float value)
{
  // TODO: mcp4922 call
}