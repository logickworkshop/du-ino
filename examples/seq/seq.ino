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
    main_selected = 2;
    top_selected = 0;
    display_changed = false;

    // draw top line
    display->draw_du_logo_sm(0, 0, DUINO_SSD1306::White);
    display->draw_text(16, 0, "SEQ", DUINO_SSD1306::White);

    // draw save box
    display->fill_rect(122, 1, 5, 5, DUINO_SSD1306::White);

    // load settings
    load_params(0, stage_pitch, 8);
    load_params(8, stage_steps, 8);
    load_params(16, stage_gate, 8);
    load_params(24, stage_slew, 1);
    load_params(25, stage_count, 1);
    load_params(26, diradd_mode, 1);
    load_params(27, slew_rate, 1);
    load_params(28, gate_time, 1);
    load_params(29, clock_bpm, 1);

    // verify settings and export to function parameters
    for(uint8_t i = 0; i < 8; ++i)
    {
      if(stage_pitch[i] < 0)
      {
        stage_pitch[i] = 0;
      }
      if(stage_pitch[i] > 119)
      {
        stage_pitch[i] = 119;
      }
      seq_values.stage_cv[i] = note_to_cv(stage_pitch[i]);
    }

    for(uint8_t i = 0; i < 8; ++i)
    {
      if(stage_steps[i] < 1 || stage_steps[i] > 8)
      {
        stage_steps[i] = 8;
      }
      seq_values.stage_steps[i] = stage_steps[i];
    }
    
    for(uint8_t i = 0; i < 8; ++i)
    {
      if(stage_gate[i] < 0 || stage_gate[i] > 5)
      {
        stage_gate[i] = 2;
      }
      seq_values.stage_gate[i] = (GateMode)stage_gate[i];
    }

    for(uint8_t i = 0; i < 8; ++i)
    {
      seq_values.stage_slew[i] = (bool)((stage_slew >> i) & 1);
    }

    if(stage_count < 1 || stage_count > 8)
    {
      stage_count = 8;
    }

    seq_values.stage_count = stage_count;
    seq_values.diradd_mode = (bool)diradd_mode;
    seq_values.slew_rate = (float)slew_rate / 255.0;
    seq_values.gate_time = (float)gate_time / 255.0;
    
    if(clock_bpm < 0 || clock_bpm > 30)
    {
      clock_bpm = 0;
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

    invert_current_selection();
    display->display();
  }

  virtual void loop()
  {
    // handle encoder button press
    DUINO_Encoder::Button b = encoder->get_button();
    if(b == DUINO_Encoder::DoubleClicked)
    {
      // switch main selection
      invert_current_selection();
      main_selected++;
      main_selected %= 6;
      invert_current_selection();
      display_changed = true;
    }
    else if(b == DUINO_Encoder::Clicked)
    {
      // switch local selection
      invert_current_selection();
      switch(main_selected)
      {
        case 0: // save
          save_params(0, stage_pitch, 8);
          save_params(8, stage_steps, 8);
          save_params(16, stage_gate, 8);
          save_params(24, stage_slew, 1);
          save_params(25, stage_count, 1);
          save_params(26, diradd_mode, 1);
          save_params(27, slew_rate, 1);
          save_params(28, gate_time, 1);
          save_params(29, clock_bpm, 1);
          break;
        case 1: // top
          top_selected++;
          top_selected %= 5;
          break;
        default: // stages
          stage_selected++;
          stage_selected %= 8;
          break;
      }
      invert_current_selection();
      display_changed = true;
    }

    // handle encoder spin
    int16_t v = encoder->get_value();
    if(v)
    {
      switch(main_selected)
      {
        case 0: // save
          break;
        case 1: // top
          switch(top_selected)
          {
            case 0: // count
              stage_count += v;
              if(stage_count < 1)
              {
                stage_count = 1;
              }
              else if(stage_count > 8)
              {
                stage_count = 8;
              }
              seq_values.stage_count = stage_count;
              display->fill_rect(37, 1, 7, 9, DUINO_SSD1306::White);
              display->draw_char(38, 2, '0' + stage_count, DUINO_SSD1306::Black);
              break;
            case 1: // dir/add
              if(v < 0)
              {
                diradd_mode = 0;
              }
              else if(v > 0)
              {
                diradd_mode = 1;
              }
              seq_values.diradd_mode = (bool)diradd_mode;
              display->fill_rect(46, 1, 7, 9, DUINO_SSD1306::White);
              display->draw_char(47, 2, diradd_mode ? 'A' : 'D', DUINO_SSD1306::Black);
              break;
            case 2: // slew
              // TODO: slew
              display->fill_rect(55, 1, 20, 9, DUINO_SSD1306::White);
              break;
            case 3: // gate
              // TODO: gate
              display->fill_rect(77, 1, 20, 9, DUINO_SSD1306::White);
              break;
            case 4: // clock
              clock_bpm += v;
              if(clock_bpm < 0)
              {
                clock_bpm = 0;
              }
              else if(clock_bpm > 30)
              {
                clock_bpm = 30;
              }
              seq_values.clock_period = bpm_to_ms(clock_bpm);
              seq_values.clock_ext = !(bool)clock_bpm;
              display->fill_rect(99, 1, 19, 9, DUINO_SSD1306::White);
              display_clock(100, 2, clock_bpm, DUINO_SSD1306::Black);
              break;
          }
          break;
        case 2: // pitch
          stage_pitch[stage_selected] += v;
          if(stage_pitch[stage_selected] < 0)
          {
            stage_pitch[stage_selected] = 0;
          }
          else if(stage_pitch[stage_selected] > 119)
          {
            stage_pitch[stage_selected] = 119;
          }
          seq_values.stage_cv[stage_selected] = note_to_cv(stage_pitch[stage_selected]);
          display->fill_rect(16 * stage_selected, 10, 16, 15, DUINO_SSD1306::White);
          display_note(16 * stage_selected, 9, stage_pitch[stage_selected], DUINO_SSD1306::Black);
          break;
        case 3: // steps
          stage_steps[stage_selected] += v;
          if(stage_steps[stage_selected] < 1)
          {
            stage_steps[stage_selected] = 1;
          }
          else if(stage_steps[stage_selected] > 8)
          {
            stage_steps[stage_selected] = 8;
          }
          seq_values.stage_steps[stage_selected] = stage_steps[stage_selected];
          display->fill_rect(16 * stage_selected, 25, 16, 17, DUINO_SSD1306::White);
          for(uint8_t j = 0; j < stage_steps[stage_selected]; ++j)
          {
            display->draw_hline(16 * stage_selected + 2, 40 - j * 2, 5, DUINO_SSD1306::Black);
            display->draw_hline(16 * stage_selected + 9, 40 - j * 2, 5, DUINO_SSD1306::Black);
          }
          break;
        case 4: // gate
          stage_gate[stage_selected] += v;
          if(stage_gate[stage_selected] < 0)
          {
            stage_gate[stage_selected] = 0;
          }
          else if(stage_gate[stage_selected] > 5)
          {
            stage_gate[stage_selected] = 5;
          }
          seq_values.stage_gate[stage_selected] = (GateMode)stage_gate[stage_selected];
          display->fill_rect(16 * stage_selected, 42, 16, 16, DUINO_SSD1306::White);
          display->draw_icon_16(16 * stage_selected, 43, gate_mode_icons, (GateMode)stage_gate[stage_selected],
              DUINO_SSD1306::Black);
          break;
        case 5: // slew
          if(v < 0)
          {
            stage_slew |= (1 << stage_selected);
          }
          else if(v > 0)
          {
            stage_slew &= ~(1 << stage_selected);
          }
          seq_values.stage_slew[stage_selected] = (bool)((stage_slew >> stage_selected) & 1);
          display->fill_rect(16 * stage_selected + 1, 59, 14, 4, DUINO_SSD1306::Black);
          display->fill_rect(16 * stage_selected + 2 + 6 * ((stage_slew >> stage_selected) & 1), 60, 6, 2,
              DUINO_SSD1306::White);  
          break;
      }

      display_changed = (bool)main_selected;
    }

    if(display_changed)
    {
      display->display();
      display_changed = false;
    }
  }

 private:
  float note_to_cv(int8_t note)
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
      display->draw_text(x, y, "EXT", color);
    }
    else
    {
      display->draw_char(x, y, '0' + bpm / 10, color);
      display->draw_char(x + 6, y, '0' + bpm % 10, color);
      display->draw_char(x + 12, y, '0', color);
    }
  }

  void display_note(int16_t x, int16_t y, int8_t note, DUINO_SSD1306::SSD1306Color color)
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

  void invert_current_selection()
  {
    switch(main_selected)
    {
      case 0: // save
        display->fill_rect(121, 0, 7, 7, DUINO_SSD1306::Inverse);
        break;
      case 1: // top
        switch(top_selected)
        {
          case 0: // count
            display->fill_rect(37, 1, 7, 9, DUINO_SSD1306::Inverse);
            break;
          case 1: // dir/add
            display->fill_rect(46, 1, 7, 9, DUINO_SSD1306::Inverse);
            break;
          case 2: // slew
            display->fill_rect(55, 1, 20, 9, DUINO_SSD1306::Inverse);
            break;
          case 3: // gate
            display->fill_rect(77, 1, 20, 9, DUINO_SSD1306::Inverse);
            break;
          case 4: // clock
            display->fill_rect(99, 1, 19, 9, DUINO_SSD1306::Inverse);
            break;
        }
        break;
      case 2: // pitch
        display->fill_rect(16 * stage_selected, 10, 16, 15, DUINO_SSD1306::Inverse);
        break;
      case 3: // steps
        display->fill_rect(16 * stage_selected, 25, 16, 17, DUINO_SSD1306::Inverse);
        break;
      case 4: // gate
        display->fill_rect(16 * stage_selected, 42, 16, 16, DUINO_SSD1306::Inverse);
        break;
      case 5: // slew
        display->fill_rect(16 * stage_selected, 58, 16, 6, DUINO_SSD1306::Inverse);
        break;
    }
  }

  uint8_t main_selected;  // 0 - save, 1 - top, 2 - pitch, 3 - steps, 4 - gate, 5 - slew
  uint8_t top_selected;   // 0 - count, 1 - dir/add, 2 - slew, 3 - gate, 4 - clock
  uint8_t stage_selected;

  bool display_changed;

  int8_t stage_pitch[8];
  int8_t stage_steps[8];
  int8_t stage_gate[8];
  uint8_t stage_slew;
  int8_t stage_count;
  uint8_t diradd_mode;
  uint8_t slew_rate;
  uint8_t gate_time;
  int8_t clock_bpm;
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
