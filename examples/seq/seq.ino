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
 * DU-INO DU-SEQ Emulator
 * Aaron Mavrinac <aaron@logick.ca>
 */

#include <du-ino_function.h>
#include <du-ino_interface.h>
#include <TimerOne.h>
#include <avr/pgmspace.h>

enum GateMode {
  GATE_NONE = 0,
  GATE_1SHT = 1,
  GATE_REPT = 2,
  GATE_LONG = 3,
  GATE_EXT1 = 4,
  GATE_EXT2 = 5
};

enum Intonation {
  IN,
  IF,
  IS
};

static const unsigned char gate_mode_icons[] PROGMEM = {
  0x00, 0x00, 0x06, 0x60, 0x0E, 0x70, 0x1C, 0x38, 0x38, 0x1C, 0x70, 0x0E, 0xE0, 0x07, 0xC0, 0x03, // GATE_NONE
  0xC0, 0x03, 0xE0, 0x07, 0x70, 0x0E, 0x38, 0x1C, 0x1C, 0x38, 0x0E, 0x70, 0x06, 0x60, 0x00, 0x00,
  0x00, 0x00, 0xC0, 0x03, 0x30, 0x0C, 0x88, 0x11, 0xE4, 0x27, 0xF4, 0x2F, 0xF2, 0x4F, 0xFA, 0x5F, // GATE_1SHT
  0xFA, 0x5F, 0xF2, 0x4F, 0xF4, 0x2F, 0xE4, 0x27, 0x88, 0x11, 0x30, 0x0C, 0xC0, 0x03, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x78, 0x00, 0x7E, 0x00, 0x7F, 0x80, 0x79, 0x80, 0x00, 0x9E, 0x78, 0xFE, 0x7F, // GATE_REPT
  0xFE, 0x7F, 0x9E, 0x78, 0x80, 0x00, 0x80, 0x79, 0x00, 0x7F, 0x00, 0x7E, 0x00, 0x78, 0x00, 0x00,
  0x00, 0x00, 0x80, 0x01, 0xC0, 0x03, 0xC0, 0x03, 0xC0, 0x03, 0xC0, 0x03, 0xC0, 0x03, 0xC0, 0x03, // GATE_LONG
  0xC0, 0x03, 0xC0, 0x03, 0xC0, 0x03, 0xC0, 0x03, 0xC0, 0x03, 0xC0, 0x03, 0x80, 0x01, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x1F, 0x00, 0x20, 0x00, 0x40, 0x80, 0x40, 0x80, 0x43, 0x80, 0x4F, 0xFC, 0x7F, // GATE_EXT1
  0xFC, 0x7F, 0x80, 0x4F, 0x80, 0x43, 0xA4, 0x40, 0x3E, 0x40, 0x20, 0x20, 0x00, 0x1F, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x1F, 0x00, 0x20, 0x00, 0x40, 0x80, 0x40, 0x80, 0x43, 0x80, 0x4F, 0xFC, 0x7F, // GATE_EXT2
  0xFC, 0x7F, 0x80, 0x4F, 0xA4, 0x43, 0xB2, 0x40, 0x2A, 0x40, 0x24, 0x20, 0x00, 0x1F, 0x00, 0x00
};

static const unsigned char semitone_lt[12] = {'C', 'C', 'D', 'E', 'E', 'F', 'F', 'G', 'G', 'A', 'B', 'B'};
static const Intonation semitone_in[12] = {IN, IS, IN, IF, IN, IN, IS, IN, IS, IN, IF, IN};

struct DU_SEQ_Values {
  float stage_cv[8];
  uint8_t stage_steps[8];
  GateMode stage_gate[8];
  bool stage_slew[8];
  uint8_t stage_count;
  bool diradd_mode;
  float slew_rate;
  float gate_time;
  float clock_period;
  bool clock_ext;
};

DU_SEQ_Values seq_values;

void clock_isr();
void reset_isr();
void timer_isr();

class DU_SEQ_Function : public DUINO_Function {
 public:
  DU_SEQ_Function() : DUINO_Function(0b0111110000) { }
  
  virtual void setup()
  {
    gt_attach_interrupt(GT3, clock_isr, CHANGE);
    gt_attach_interrupt(GT4, reset_isr, RISING);
  }

  virtual void loop()
  {
  }
};

class DU_SEQ_Interface : public DUINO_Interface {
 public:
  virtual void setup()
  {
    // initialize interface
    display_changed = false;

    // draw top line
    display->draw_du_logo_sm(0, 0, DUINO_SSD1306::White);
    display->draw_text(16, 0, "SEQ", DUINO_SSD1306::White);

    // draw save box
    display->fill_rect(122, 1, 5, 5, DUINO_SSD1306::White);

    // load settings
    load_params(0, stage_pitch, 8);
    for(uint8_t i = 0; i < 8; ++i)
    {
      if(stage_pitch[i] > 119)
      {
        stage_pitch[i] = 119;
      }
      seq_values.stage_cv[i] = note_to_cv(stage_pitch[i]);
    }
    load_params(8, stage_steps, 8);
    for(uint8_t i = 0; i < 8; ++i)
    {
      if(stage_steps[i] < 1 || stage_steps[i] > 8)
      {
        stage_steps[i] = 8;
      }
      seq_values.stage_steps[i] = stage_steps[i];
    }
    load_params(16, stage_gate, 8);
    for(uint8_t i = 0; i < 8; ++i)
    {
      if(stage_gate[i] > 5)
      {
        stage_gate[i] = 5;
      }
      seq_values.stage_gate[i] = (GateMode)stage_gate[i];
    }
    load_params(24, stage_slew, 1);
    load_params(25, stage_count, 1);
    if(stage_count < 1 || stage_count > 8)
    {
      stage_count = 8;
    }
    seq_values.stage_count = stage_count;
    load_params(26, diradd_mode, 1);
    seq_values.diradd_mode = (bool)diradd_mode;
    load_params(27, slew_rate, 1);
    seq_values.slew_rate = (float)slew_rate / 255.0;
    load_params(28, gate_time, 1);
    seq_values.gate_time = (float)gate_time / 255.0;
    load_params(29, clock_bpm, 1);
    if(clock_bpm > 30)
    {
      clock_bpm = 30;
    }
    seq_values.clock_period = bpm_to_ms(clock_bpm);
    seq_values.clock_ext = !(bool)clock_bpm;

    // draw global elements
    display->draw_char(38, 2, '0' + stage_count, DUINO_SSD1306::White);
    display->draw_char(47, 2, diradd_mode ? 'A' : 'D', DUINO_SSD1306::White);
    for(uint8_t i = 0; i < 2; ++i)
    {
      display->draw_vline(56 + i * 22, 2, 7, DUINO_SSD1306::White);
      display->draw_hline(57 + i * 22, 2, 16, DUINO_SSD1306::White);
      display->draw_hline(57 + i * 22, 8, 16, DUINO_SSD1306::White);
      display->draw_vline(73 + i * 22, 2, 7, DUINO_SSD1306::White);
    }
    display_clock(100, 2, clock_bpm, DUINO_SSD1306::White);

    // draw step elements
    for(uint8_t i = 0; i < 8; ++i)
    {
      // pitch
      display_note(16 * i, 9, stage_pitch[i], DUINO_SSD1306::White);
      // steps
      for(uint8_t j = 0; j < stage_steps[i]; ++j)
      {
        display->draw_hline(16 * i + 2, 40 - j * 2, 5, DUINO_SSD1306::White);
        display->draw_hline(16 * i + 9, 40 - j * 2, 5, DUINO_SSD1306::White);
      }
      // gate mode
      display->draw_icon_16(16 * i, 43, gate_mode_icons, (GateMode)stage_gate[i], DUINO_SSD1306::White);
      // slew
      display->fill_rect(16 * i + 1, 59, 14, 4, DUINO_SSD1306::White);
      display->fill_rect(16 * i + 2 + 6 * (~(stage_slew >> i) & 1), 60, 6, 2, DUINO_SSD1306::Black);
    }

    display->display();
  }

  virtual void loop()
  {

    if(display_changed)
    {
      display->display();
      display_changed = false;
    }
  }

 private:
  float note_to_cv(uint8_t note)
  {
    return ((float)note - 36.0) / 12.0;
  }

  float bpm_to_ms(uint8_t bpm)
  {
    return 600000.0 / (float)bpm;
  }

  void display_clock(int16_t x, int16_t y, uint8_t bpm, DUINO_SSD1306::SSD1306Color color)
  {
    if(bpm == 0)
    {
      display->draw_text(x, y, "EXT", DUINO_SSD1306::White);
    }
    else
    {
      display->draw_char(x, y, '0' + bpm / 10, DUINO_SSD1306::White);
      display->draw_char(x + 6, y, '0' + bpm % 10, DUINO_SSD1306::White);
      display->draw_char(x + 12, y, '0', DUINO_SSD1306::White);
    }
  }

  void display_note(int16_t x, int16_t y, uint8_t note, DUINO_SSD1306::SSD1306Color color)
  {
    // draw octave
    display->draw_char(x + 9, y + 8, '0' + note / 12, color);

    // draw note
    display->draw_char(x + 2, y + 5, semitone_lt[note % 12], color);
    
    // draw intonation symbol
    switch(semitone_in[note % 12])
    {
      case IF:
        display->draw_vline(x + 9, y + 2, 5, color);
        display->draw_pixel(x + 10, y + 4, color);
        display->draw_pixel(x + 10, y + 6, color);
        display->draw_vline(x + 11, y + 5, 2, color);
        break;
      case IS:
        display->draw_vline(x + 9, y + 2, 5, color);
        display->draw_pixel(x + 10, y + 3, color);
        display->draw_pixel(x + 10, y + 5, color);
        display->draw_vline(x + 11, y + 2, 5, color);
        break;
    }
  }

  bool display_changed;

  uint8_t stage_pitch[8];
  uint8_t stage_steps[8];
  uint8_t stage_gate[8];
  uint8_t stage_slew;
  uint8_t stage_count;
  uint8_t diradd_mode;
  uint8_t slew_rate;
  uint8_t gate_time;
  uint8_t clock_bpm;
};

DU_SEQ_Function * function;
DU_SEQ_Interface * interface;

void clock_isr()
{
}

void reset_isr()
{
}

void timer_isr()
{
  interface->timer_isr();
}

void setup()
{
  function = new DU_SEQ_Function();
  interface = new DU_SEQ_Interface();

  function->begin();
  interface->begin();

  Timer1.initialize(1000);
  Timer1.attachInterrupt(timer_isr);
}

void loop()
{
  function->loop();
  interface->loop();
}
