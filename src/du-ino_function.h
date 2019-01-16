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

#ifndef DUINO_FUNCTION_H_
#define DUINO_FUNCTION_H_

#include "Arduino.h"
#include "du-ino_sh1106.h"
#include "du-ino_encoder.h"

class DUINO_MCP4922;
class DUINO_Widget;

/** Main function controller base class. */
class DUINO_Function
{
public:
  enum Jack
  {
    GT1 = 0,
    GT2 = 1,
    GT3 = 2,
    GT4 = 3,
    CO1 = 4,
    CO2 = 5,
    CO3 = 7,
    CO4 = 6,
    CI1 = 8,
    CI2 = 9,
    CI3 = 10,
    CI4 = 11
  };

  /**
   * Constructor.
   *
   * \param sc Switch configuration (see set_switch_config() for details).
   */
  DUINO_Function(uint8_t sc);

  /**
   * Initialize the function.
   */
  void begin();

  /**
   * Setup hook; should be overridden by subclasses and called in the main Arduino setup().
   */
  virtual void setup() {}

  /**
   * Loop hook; should be overriden by subclasses and called in the main Arduino loop().
   */
  virtual void loop() {}

  /**
   * UI widget setup; normally called in setup() override after widgets are initialized.
   *
   * \param top Top-level widget (usually a container).
   */
  void widget_setup(DUINO_Widget * top);

  /**
   * UI widget loop; normally called in loop() override.
   */
  void widget_loop();

  /**
   * Read a gate/trigger input.
   *
   * \param jack The input jack to read.
   * \return The digital value of the input.
   */
  bool gt_read(Jack jack);

  /**
   * Read a gate/trigger input, with software debounce.
   *
   * \param jack The input jack to read.
   * \return The digital value of the input.
   */
  bool gt_read_debounce(Jack jack);

  /**
   * Output a gate or trigger signal.
   *
   * \param jack The output jack.
   * \param on The value of the gate/trigger signal (on if true, off if false).
   * \param trig Trigger; if true, output the specified value briefly, then output the opposite value.
   */
  void gt_out(Jack jack, bool on, bool trig = false);

  /**
   * Output a gate or trigger signal to multiple jacks simultaneously.
   *
   * \param jacks The output jacks, as a bitfield; e.g. (1 << GT3) | (1 << GT4).
   * \param on The value of the gate/trigger signal (on if true, off if false).
   * \param trig Trigger; if true, output the specified value briefly, then output the opposite value.
   */
  void gt_out_multi(uint8_t jacks, bool on, bool trig = false);

  /**
   * Read a CV input.
   *
   * \param jack The input jack to read.
   * \return The CV value, in volts.
   */
  float cv_read(Jack jack);

  /**
   * Output a CV value.
   *
   * \param jack The output jack.
   * \param value The CV value, in volts.
   */
  void cv_out(Jack jack, float value);

  /**
   * Hold CV outputs; used to set multiple values with cv_out() then release them simultaneously.
   *
   * \param state If true, hold the current CV values on all four CV jacks until called with false.
   */
  void cv_hold(bool state);

  /**
   * Attach an interrupt callback to a jack with hardware interrupt capability.
   *
   * \param jack The interrupt jack (GT3 or GT4).
   * \param isr The interrupt service routine function pointer.
   * \param mode When the interrupt should be triggered (LOW, CHANGE, RISING, FALLING).
   */
  void gt_attach_interrupt(Jack jack, void (*isr)(void), int mode);

  /**
   * Detach an interrupt callback from a jack.
   *
   * \param jack The interrupt jack (GT3 or GT4).
   */
  void gt_detach_interrupt(Jack jack);

  /**
   * Set the switch configuration for the function (must match the physical switches).
   *
   * The switches have the following routing effects:
   *
   * SWITCH   NC (0)        NO (1)
   * ------   ----------    -----------
   * SG1      GT1 output    GT1 input
   * SG2      GT2 output    GT2 input
   * SG3      GT3 output    GT3 input
   * SG4      GT4 output    GT4 input
   * SC1      AD1+ = CI1    AD1+ = DAC1
   * SC2      AD1- = CI2    AD1- = DAC2
   * SC3      AD2+ = CI3    AD2+ = DAC3
   * SC4      AD2- = CI4    AD2- = DAC4
   *
   * \param sc Switch configuration, 0b{SC4 .. SC1}{SG4 .. SG1}.
   */
  void set_switch_config(uint8_t sc);

 protected:
  inline float cv_analog_read(uint8_t pin);

  DUINO_MCP4922 * dac_[2];

  DUINO_Widget * top_level_widget_;

  bool saved_;
  uint8_t switch_config_;
};

#endif // DUINO_FUNCTION_H_
