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
 * DU-INO Quad LFO
 * Aaron Mavrinac <aaron@logick.ca>
 *
 * JACK    FUNCTION
 * ----    --------
 * GT1 I - hold in
 * GT2 I - hold on zero in
 * GT3 I - sync/tap in
 * GT4 I - reset in
 * CI1   - LFO1 frequency CV
 * CI2   - LFO2 frequency CV
 * CI3   - LFO3 frequency CV
 * CI4   - LFO4 frequency CV
 * OFFST -
 * CO1   - LFO1 out
 * CO2   - LFO2 out
 * CO3   - LFO3 out
 * CO4   - LFO4 out
 * FNCTN - (LFO1 - LFO2) * (LFO3 - LFO4)
 *
 * SWITCH CONFIGURATION
 * --------------------
 * SG2    [^][^]    SG1
 * SG4    [^][^]    SG3
 * SC2    [^][^]    SC1
 * SC4    [^][^]    SC3
 */

#include <du-ino_function.h>
#include <avr/pgmspace.h>

static const unsigned char icons[] PROGMEM =
{
  0x3e, 0x02, 0x02, 0x3e, 0x20, 0x20, 0x3e,  // square wave
  0x1c, 0x02, 0x02, 0x1c, 0x20, 0x20, 0x1c,  // sine wave
  0x20, 0x10, 0x08, 0x04, 0x3e, 0x10, 0x08,  // saw wave
  0x10, 0x08, 0x04, 0x02, 0x04, 0x08, 0x10,  // triangle wave
  0x3e, 0x6f, 0x59, 0x5f, 0x59, 0x6f, 0x3e,  // normal
  0x3e, 0x7b, 0x4d, 0x7d, 0x4d, 0x7b, 0x3e   // inverted
};

class DU_LFO_Function : public DUINO_Function
{
public:
  DU_LFO_Function() : DUINO_Function(0b11111111) { }

  virtual void function_setup()
  {
  }

  virtual void function_loop()
  {
  }
};

DU_LFO_Function * function;

void setup()
{
  function = new DU_LFO_Function();

  function->begin();
}

void loop()
{
  function->function_loop();
}
