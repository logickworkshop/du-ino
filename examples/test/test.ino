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
 * DU-INO Test Function
 * Aaron Mavrinac <aaron@logick.ca>
 */

#include <du-ino_function.h>

#define GT_INT_DISPLAY_TIME   1000

volatile uint8_t gt_state;
volatile bool gt3_retrigger, gt4_retrigger;
volatile unsigned long gt3_retrigger_time, gt4_retrigger_time;

void gt3_isr();
void gt4_isr();

class DU_Test_Function : public DUINO_Function
{
public:
  DU_Test_Function() : DUINO_Function(0b00001111) { }
  
  virtual void setup()
  {
    gt_attach_interrupt(GT3, gt3_isr, CHANGE);
    gt_attach_interrupt(GT4, gt4_isr, CHANGE);

    calibration_value = calibration_value_last = 0;
    gt = gt_io = gt_io_last = false;

    Display.draw_logick_logo(0, 0, DUINO_SH1106::White);
    Display.draw_text(42, 3, "DU-INO TESTER", DUINO_SH1106::White);

    // draw voltage values
    Display.draw_text(0, 17, "+10V", DUINO_SH1106::White);
    Display.draw_text(12, 37, "0V", DUINO_SH1106::White);
    Display.draw_text(0, 57, "-10V", DUINO_SH1106::White);

    // draw markers
    Display.draw_hline(26, 20, 10, DUINO_SH1106::White);
    Display.draw_hline(26, 40, 10, DUINO_SH1106::White);
    Display.draw_hline(26, 60, 10, DUINO_SH1106::White);
    for (uint8_t i = 0; i < 9; ++i)
    {
      Display.draw_hline(30, 22 + 2 * i, 6, DUINO_SH1106::White);
      Display.draw_hline(30, 42 + 2 * i, 6, DUINO_SH1106::White);
    }

    // draw arrow
    display_calibration_arrow();

    // draw CV section
    Display.draw_text(48, 17, "CV IN", DUINO_SH1106::White);
    Display.draw_hline(48, 25, 35, DUINO_SH1106::White);

    // draw GT section
    Display.draw_text(96, 17, "GT", DUINO_SH1106::White);
    display_gtio_arrow();
    Display.draw_hline(96, 25, 24, DUINO_SH1106::White);
    for (uint8_t i = 0; i < 4; ++i)
    {
      Display.draw_char(96, 27 + 10 * i, 0x31 + i, DUINO_SH1106::White);
    }

    Display.display();
  }

  virtual void loop()
  {
    if (gt3_retrigger)
    {
      if(millis() - gt3_retrigger_time > GT_INT_DISPLAY_TIME)
      {
        gt3_retrigger = false;
      }
    }
    if (gt4_retrigger)
    {
      if(millis() - gt4_retrigger_time > GT_INT_DISPLAY_TIME)
      {
        gt4_retrigger = false;
      }
    }

    cv[0] = cv_read(CI1);
    cv[1] = cv_read(CI2);
    cv[2] = cv_read(CI3);
    cv[3] = cv_read(CI4);

    cv_out(CO1, (float)calibration_value);
    cv_out(CO2, (float)calibration_value);
    cv_out(CO3, (float)calibration_value);
    cv_out(CO4, (float)calibration_value);

    if (gt_io != gt_io_last)
    {
      gt_io_last = gt_io;
      set_switch_config(gt_io ? 0b00000000 : 0b00001111);
    }

    if (gt_io)
    {
      gt_out_multi(0x0F, gt);
    }
    else
    {
      if (gt_read(GT1))
      {
        gt_state |= 1U;
      }
      else
      {
        gt_state &= ~1U;
      }
      if (gt_read(GT2))
      {
        gt_state |= 2U;
      }
      else
      {
        gt_state &= ~2U;
      }
    }

    // handle encoder button press
    DUINO_Encoder::Button b = Encoder.get_button();
    if (b == DUINO_Encoder::DoubleClicked)
    {
      gt_io = !gt_io;
      display_gtio_arrow();
    }
    else if (b == DUINO_Encoder::Held)
    {
      gt = true;
    }
    else if (b == DUINO_Encoder::Released)
    {
      gt = false;
    }

    // handle encoder spin
    calibration_value += Encoder.get_value();
    if (calibration_value > 10)
    {
      calibration_value = 10;
    }
    if (calibration_value < -10)
    {
      calibration_value = -10;
    }
    if (calibration_value != calibration_value_last)
    {
      calibration_value_last = calibration_value;
      display_calibration_arrow();
    }

    // display CV input values
    display_cv_in();

    // display GT input/output states
    display_gtio();

    // update display
    Display.display();
  }

private:
  void display_calibration_arrow()
  {
    Display.fill_rect(37, 17, 5, 47, DUINO_SH1106::Black);
    Display.draw_char(37, 37 - 2 * calibration_value, 0x11, DUINO_SH1106::White);
  }

  void display_gtio_arrow()
  {
    Display.fill_rect(114, 17, 5, 7, DUINO_SH1106::Black);
    Display.draw_char(114, 17, 0x11 - (unsigned char)gt_io, DUINO_SH1106::White);
  }

  void display_cv_in()
  {
    Display.fill_rect(48, 27, 35, 37, DUINO_SH1106::Black);
    for (uint8_t i = 0; i < 4; ++i)
    {
      char value[7];
      dtostrf(cv[i], 6, 2, value);
      Display.draw_text(48, 27 + 10 * i, value, DUINO_SH1106::White);
    }
  }

  void display_gtio()
  {
    Display.fill_rect(104, 27, 5, 37, DUINO_SH1106::Black);
    Display.fill_rect(120, 47, 5, 17, DUINO_SH1106::Black);

    // input & output for GT1 - GT4
    for (uint8_t i = 0; i < 4; ++i)
    {
      if ((i == 2 && gt3_retrigger) || (i == 3) && gt4_retrigger)
      {
        Display.draw_char(104, 27 + 10 * i, 0x0F, DUINO_SH1106::White);
      }
      else if ((gt_io && gt) || (!gt_io && (gt_state & (1U << i))))
      {
        Display.draw_char(104, 27 + 10 * i, 0x04, DUINO_SH1106::White);
      }
    }
  }

  int8_t calibration_value, calibration_value_last;
  bool gt, gt_io, gt_io_last;
  float cv[4];
};

DU_Test_Function * function;

void gt3_isr()
{
  if (function->gt_read_debounce(DUINO_Function::GT3))
  {
    gt_state |= 4U;
    gt3_retrigger = true;
    gt3_retrigger_time = millis();
  }
  else
  {
    gt_state &= ~4U;
  }
}

void gt4_isr()
{
  if (function->gt_read_debounce(DUINO_Function::GT4))
  {
    gt_state |= 8U;
    gt4_retrigger = true;
    gt4_retrigger_time = millis();
  }
  else
  {
    gt_state &= ~8U;
  }
}

void setup()
{
  function = new DU_Test_Function();

  gt_state = 0;
  gt3_retrigger = gt4_retrigger = false;
  gt3_retrigger_time = gt4_retrigger_time = 0;

  function->begin();
}

void loop()
{
  function->loop();
}
