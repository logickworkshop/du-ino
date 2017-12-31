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
#include <du-ino_interface.h>

volatile bool gt3_state, gt3_retrigger, gt4_state, gt4_retrigger;
int8_t calibration_value;
bool gt, gt_io;
float cv[4];

void gt3_isr();
void gt4_isr();

class DU_Test_Function : public DUINO_Function {
 public:
  DU_Test_Function() : DUINO_Function(0b000001111) { }
  
  virtual void setup()
  {
    gt3_state = gt3_retrigger = false;
    gt4_state = gt4_retrigger = false;
    gt_attach_interrupt(GT3, gt3_isr, CHANGE);
    gt_attach_interrupt(GT4, gt4_isr, CHANGE);

    gt_io_last = gt_io;
  }

  virtual void loop()
  {
    cv[0] = cv_read(CI1);
    cv[1] = cv_read(CI2);
    cv[2] = cv_read(CI3);
    cv[3] = cv_read(CI4);

    cv_out(CO1, float(calibration_value));
    cv_out(CO2, float(calibration_value));
    cv_out(CO3, float(calibration_value));
    cv_out(CO4, float(calibration_value));

    if(gt_io != gt_io_last)
    {
      gt_io_last = gt_io;
      set_switch_config(gt_io ? 0b0000110000 : 0b0000001111);
    }

    gt_out(GT_ALL, gt);
  }

 private:
  bool gt_io_last;
};

class DU_Test_Interface : public DUINO_Interface {
 public:
  virtual void setup()
  {
    display_changed = false;
    last_calibration_value = 0;

    display->draw_logick_logo(0, 0, DUINO_SSD1306::White);
    display->draw_text(42, 3, "DU-INO TESTER", DUINO_SSD1306::White);

    // draw voltage values
    display->draw_text(0, 17, "+10V", DUINO_SSD1306::White);
    display->draw_text(12, 37, "0V", DUINO_SSD1306::White);
    display->draw_text(0, 57, "-10V", DUINO_SSD1306::White);

    // draw markers
    display->draw_hline(26, 20, 10, DUINO_SSD1306::White);
    display->draw_hline(26, 40, 10, DUINO_SSD1306::White);
    display->draw_hline(26, 60, 10, DUINO_SSD1306::White);
    for(uint8_t i = 0; i < 9; ++i)
    {
      display->draw_hline(30, 22 + 2 * i, 6, DUINO_SSD1306::White);
      display->draw_hline(30, 42 + 2 * i, 6, DUINO_SSD1306::White);
    }

    // draw arrow
    display_calibration_arrow();

    // draw CV section
    display->draw_text(48, 17, "CV IN", DUINO_SSD1306::White);
    display->draw_hline(48, 25, 35, DUINO_SSD1306::White);

    // draw GT section
    display->draw_text(96, 17, "GT", DUINO_SSD1306::White);
    display_gtio_arrow();
    display->draw_hline(96, 25, 24, DUINO_SSD1306::White);
    for(uint8_t i = 0; i < 4; ++i)
    {
      display->draw_char(96, 27 + 10 * i, 0x31 + i, DUINO_SSD1306::White);
    }
    display->draw_char(112, 47, '5', DUINO_SSD1306::White);
    display->draw_char(112, 57, '6', DUINO_SSD1306::White);

    display->display();
  }

  virtual void loop()
  {
    // handle encoder button press
    DUINO_Encoder::Button b = encoder->get_button();
    if(b == DUINO_Encoder::DoubleClicked)
    {
      gt_io = !gt_io;
      display_gtio_arrow();
    }
    else if(b == DUINO_Encoder::Held)
    {
      // TODO: digital output control
    }

    // handle encoder spin
    calibration_value += encoder->get_value();
    if(calibration_value > 10)
    {
      calibration_value = 10;
    }
    if(calibration_value < -10)
    {
      calibration_value = -10;
    }
    if(calibration_value != last_calibration_value)
    {
      last_calibration_value = calibration_value;
      display_calibration_arrow();
    }

    // display CV input values
    display_cv_in();

    // update display
    display->display();
  }

 private:
  void display_calibration_arrow()
  {
    display->fill_rect(37, 17, 5, 47, DUINO_SSD1306::Black);
    display->draw_char(37, 37 - 2 * calibration_value, 0x11, DUINO_SSD1306::White);
  }

  void display_gtio_arrow()
  {
    display->fill_rect(114, 17, 5, 7, DUINO_SSD1306::Black);
    display->draw_char(114, 17, 0x11 - (unsigned char)gt_io, DUINO_SSD1306::White);
  }

  void display_cv_in()
  {
    display->fill_rect(48, 27, 35, 37, DUINO_SSD1306::Black);
    for(uint8_t i = 0; i < 4; ++i)
    {
      char value[7];
      dtostrf(cv[i], 6, 2, value);
      display->draw_text(48, 27 + 10 * i, value, DUINO_SSD1306::White);
    }
  }

  bool display_changed;
  int8_t last_calibration_value;
};

DU_Test_Function * function;
DU_Test_Interface * interface;

ENCODER_ISR(interface->encoder);

void gt3_isr()
{
  gt3_state = function->gt_read(GT3);
  if(gt3_state)
    gt3_retrigger = true;
}

void gt4_isr()
{
  gt4_state = function->gt_read(GT4);
  if(gt4_state)
    gt4_retrigger = true;
}

void setup()
{
  function = new DU_Test_Function();
  interface = new DU_Test_Interface();

  calibration_value = 0;
  gt = false;
  gt_io = false;

  function->begin();
  interface->begin();
}

void loop()
{
  function->loop();
  interface->loop();
}
