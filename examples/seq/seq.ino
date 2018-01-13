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
#include <util/atomic.h>

#define DIGITAL_THRESH 1.25 // V

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
  uint8_t slew_rate;
  uint16_t gate_ms;
  unsigned long clock_period;
  bool clock_ext;
};

volatile DU_SEQ_Values seq_values;

volatile uint8_t stage;
volatile uint8_t step;
volatile bool gate, clock_gate, retrigger;

void clock_isr();
void reset_isr();

class DU_SEQ_Function : public DUINO_Function {
 public:
  DU_SEQ_Function() : DUINO_Function(0b0111111111) { }
  
  virtual void setup()
  {
    // TODO: attach GT3 to hardware clock isr that sets clock to EXT
    gt_attach_interrupt(GT3, clock_isr, CHANGE);
    gt_attach_interrupt(GT4, reset_isr, RISING);

    gate = clock_gate = retrigger = false;
  }

  virtual void loop()
  {
    // cache stage, step, and clock gate (so that each loop is "atomic")
    ATOMIC_BLOCK(ATOMIC_FORCEON)
    {
      cached_stage = stage;
      cached_step = step;
      cached_clock_gate = clock_gate;
      cached_retrigger = retrigger;
      retrigger = false;
    }

    if(cached_retrigger)
    {
      if(!cached_step)
      {
        // drop gate and clock at start of stage
        gt_out(GT_MULTI | (1 << GT5) | (1 << GT6), false);
      }
      else
      {
        // drop clock each retrigger
        gt_out(GT6, false);
      }
      clock_time = millis();
    }

    // set gate state
    switch(seq_values.stage_gate[cached_stage])
    {
      case GATE_NONE:
        gate = false;
        break;
      case GATE_1SHT:
        if(!cached_step)
        {
          gate = partial_gate();
        }
        break;
      case GATE_REPT:
        gate = partial_gate();
        break;
      case GATE_LONG:
        if(cached_step == seq_values.stage_steps[cached_stage] - 1)
        {
          gate = partial_gate();
        }
        else
        {
          gate = true;
        }
        break;
      case GATE_EXT1:
        gate = cv_read(CI2) > DIGITAL_THRESH;
        break;
      case GATE_EXT2:
        gate = cv_read(CI3) > DIGITAL_THRESH;
        break;
    }

    // set pitch CV state
    cv_out(CO1, seq_values.stage_cv[cached_stage]);

    // set gate and clock states
    gt_out(GT5, gate);
    gt_out(GT6, cached_clock_gate);
  }

 private:
  bool partial_gate()
  {
    return (seq_values.clock_ext && cached_clock_gate)
           || cached_retrigger
           || ((millis() - clock_time) < seq_values.gate_ms);
  }

  uint8_t cached_stage, cached_step;
  bool cached_clock_gate, cached_retrigger;
  unsigned long clock_time;
};

class DU_SEQ_Interface : public DUINO_Interface {
 public:
  virtual void setup()
  {
    // initialize interface
    main_selected = 2;
    top_selected = 0;
    display_changed = false;
    gate_changed = false;
    last_gate = false;
    last_stage = 0;

    // draw top line
    display->draw_du_logo_sm(0, 0, DUINO_SSD1306::White);
    display->draw_text(16, 0, "SEQ", DUINO_SSD1306::White);

    // draw save box
    display->fill_rect(122, 1, 5, 5, DUINO_SSD1306::White);

    // load settings
    load_params(0, params.bytes, 30);

    // verify settings and export to function parameters
    for(uint8_t i = 0; i < 8; ++i)
    {
      if(params.vals.stage_pitch[i] < 0)
      {
        params.vals.stage_pitch[i] = 0;
      }
      if(params.vals.stage_pitch[i] > 119)
      {
        params.vals.stage_pitch[i] = 119;
      }
      seq_values.stage_cv[i] = note_to_cv(params.vals.stage_pitch[i]);
    }

    for(uint8_t i = 0; i < 8; ++i)
    {
      if(params.vals.stage_steps[i] < 1 || params.vals.stage_steps[i] > 8)
      {
        params.vals.stage_steps[i] = 8;
      }
      seq_values.stage_steps[i] = params.vals.stage_steps[i];
    }
    
    for(uint8_t i = 0; i < 8; ++i)
    {
      if(params.vals.stage_gate[i] < 0 || params.vals.stage_gate[i] > 5)
      {
        params.vals.stage_gate[i] = 2;
      }
      seq_values.stage_gate[i] = (GateMode)params.vals.stage_gate[i];
    }

    for(uint8_t i = 0; i < 8; ++i)
    {
      seq_values.stage_slew[i] = (bool)((params.vals.stage_slew >> i) & 1);
    }

    if(params.vals.stage_count < 1 || params.vals.stage_count > 8)
    {
      params.vals.stage_count = 8;
    }
    seq_values.stage_count = params.vals.stage_count;

    seq_values.diradd_mode = (bool)params.vals.diradd_mode;
    
    if(params.vals.slew_rate < 0 || params.vals.slew_rate > 16)
    {
      params.vals.slew_rate = 8;
    }
    seq_values.slew_rate = params.vals.slew_rate;

    if(params.vals.gate_time < 0 || params.vals.gate_time > 16)
    {
      params.vals.gate_time = 8;
    }
    seq_values.gate_ms = params.vals.gate_time * (uint16_t)(seq_values.clock_period / 8000);
    
    if(params.vals.clock_bpm < 0 || params.vals.clock_bpm > 30)
    {
      params.vals.clock_bpm = 0;
    }
    seq_values.clock_period = bpm_to_us(params.vals.clock_bpm);
    seq_values.clock_ext = !(bool)params.vals.clock_bpm;
    Timer1.initialize();
    update_clock();

    // draw global elements
    display->draw_char(38, 2, '0' + params.vals.stage_count, DUINO_SSD1306::White);
    display->draw_char(47, 2, params.vals.diradd_mode ? 'A' : 'D', DUINO_SSD1306::White);
    for(uint8_t i = 0; i < 2; ++i)
    {
      display->draw_vline(56 + i * 22, 2, 7, DUINO_SSD1306::White);
      display->draw_hline(57 + i * 22, 2, 16, DUINO_SSD1306::White);
      display->draw_hline(57 + i * 22, 8, 16, DUINO_SSD1306::White);
      display->draw_vline(73 + i * 22, 2, 7, DUINO_SSD1306::White);
    }
    display_slew_rate(57, 3, params.vals.slew_rate, DUINO_SSD1306::White);
    display_gate_time(79, 3, params.vals.gate_time, DUINO_SSD1306::White);
    display_clock(100, 2, params.vals.clock_bpm, DUINO_SSD1306::White);

    // draw step elements
    for(uint8_t i = 0; i < 8; ++i)
    {
      // pitch
      display_note(16 * i, 20, params.vals.stage_pitch[i], DUINO_SSD1306::White);
      // steps
      display_steps(16 * i, 35, params.vals.stage_steps[i], DUINO_SSD1306::White);
      // gate mode
      display->draw_icon_16(16 * i, 42, gate_mode_icons, (GateMode)params.vals.stage_gate[i], DUINO_SSD1306::White);
      // slew
      display->fill_rect(16 * i + 1, 59, 14, 4, DUINO_SSD1306::White);
      display->fill_rect(16 * i + 2 + 6 * (~(params.vals.stage_slew >> i) & 1), 60, 6, 2, DUINO_SSD1306::Black);
    }

    invert_current_selection();
    display->display_all();
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
      switch(main_selected)
      {
        case 0: // save
          save_params(0, params.bytes, 30);
          display->fill_rect(123, 2, 3, 3, DUINO_SSD1306::Black);
          break;
        case 1: // top
          invert_current_selection();
          top_selected++;
          top_selected %= 5;
          invert_current_selection();
          break;
        default: // stages
          invert_current_selection();
          stage_selected++;
          stage_selected %= 8;
          invert_current_selection();
          break;
      }
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
              params.vals.stage_count += v;
              if(params.vals.stage_count < 1)
              {
                params.vals.stage_count = 1;
              }
              else if(params.vals.stage_count > 8)
              {
                params.vals.stage_count = 8;
              }
              seq_values.stage_count = params.vals.stage_count;
              display->fill_rect(37, 1, 7, 9, DUINO_SSD1306::White);
              display->draw_char(38, 2, '0' + params.vals.stage_count, DUINO_SSD1306::Black);
              break;
            case 1: // dir/add
              if(v < 0)
              {
                params.vals.diradd_mode = 0;
              }
              else if(v > 0)
              {
                params.vals.diradd_mode = 1;
              }
              seq_values.diradd_mode = (bool)params.vals.diradd_mode;
              display->fill_rect(46, 1, 7, 9, DUINO_SSD1306::White);
              display->draw_char(47, 2, params.vals.diradd_mode ? 'A' : 'D', DUINO_SSD1306::Black);
              break;
            case 2: // slew
              params.vals.slew_rate += v;
              if(params.vals.slew_rate < 0)
              {
                params.vals.slew_rate = 0;
              }
              else if(params.vals.slew_rate > 16)
              {
                params.vals.slew_rate = 16;
              }
              seq_values.slew_rate = params.vals.slew_rate;
              display->fill_rect(57, 3, 16, 5, DUINO_SSD1306::White);
              display_slew_rate(57, 3, params.vals.slew_rate, DUINO_SSD1306::Black);
              break;
            case 3: // gate
              params.vals.gate_time += v;
              if(params.vals.gate_time < 0)
              {
                params.vals.gate_time = 0;
              }
              else if(params.vals.gate_time > 16)
              {
                params.vals.gate_time = 16;
              }
              seq_values.gate_ms = params.vals.gate_time * (uint16_t)(seq_values.clock_period / 8000);
              display->fill_rect(79, 3, 16, 5, DUINO_SSD1306::White);
              display_gate_time(79, 3, params.vals.gate_time, DUINO_SSD1306::Black);
              break;
            case 4: // clock
              params.vals.clock_bpm += v;
              if(params.vals.clock_bpm < 0)
              {
                params.vals.clock_bpm = 0;
              }
              else if(params.vals.clock_bpm > 30)
              {
                params.vals.clock_bpm = 30;
              }
              seq_values.clock_period = bpm_to_us(params.vals.clock_bpm);
              seq_values.clock_ext = !(bool)params.vals.clock_bpm;
              display->fill_rect(99, 1, 19, 9, DUINO_SSD1306::White);
              display_clock(100, 2, params.vals.clock_bpm, DUINO_SSD1306::Black);
              update_clock();
              break;
          }
          break;
        case 2: // pitch
          params.vals.stage_pitch[stage_selected] += v;
          if(params.vals.stage_pitch[stage_selected] < 0)
          {
            params.vals.stage_pitch[stage_selected] = 0;
          }
          else if(params.vals.stage_pitch[stage_selected] > 119)
          {
            params.vals.stage_pitch[stage_selected] = 119;
          }
          seq_values.stage_cv[stage_selected] = note_to_cv(params.vals.stage_pitch[stage_selected]);
          display->fill_rect(16 * stage_selected, 20, 16, 15, DUINO_SSD1306::White);
          display_note(16 * stage_selected, 20, params.vals.stage_pitch[stage_selected], DUINO_SSD1306::Black);
          break;
        case 3: // steps
          params.vals.stage_steps[stage_selected] += v;
          if(params.vals.stage_steps[stage_selected] < 1)
          {
            params.vals.stage_steps[stage_selected] = 1;
          }
          else if(params.vals.stage_steps[stage_selected] > 8)
          {
            params.vals.stage_steps[stage_selected] = 8;
          }
          seq_values.stage_steps[stage_selected] = params.vals.stage_steps[stage_selected];
          display->fill_rect(16 * stage_selected, 35, 16, 7, DUINO_SSD1306::White);
          display_steps(16 * stage_selected, 35, params.vals.stage_steps[stage_selected], DUINO_SSD1306::Black);
          break;
        case 4: // gate
          params.vals.stage_gate[stage_selected] += v;
          if(params.vals.stage_gate[stage_selected] < 0)
          {
            params.vals.stage_gate[stage_selected] = 0;
          }
          else if(params.vals.stage_gate[stage_selected] > 5)
          {
            params.vals.stage_gate[stage_selected] = 5;
          }
          seq_values.stage_gate[stage_selected] = (GateMode)params.vals.stage_gate[stage_selected];
          display->fill_rect(16 * stage_selected, 42, 16, 16, DUINO_SSD1306::White);
          display->draw_icon_16(16 * stage_selected, 42, gate_mode_icons,
              (GateMode)params.vals.stage_gate[stage_selected], DUINO_SSD1306::Black);
          break;
        case 5: // slew
          if(v < 0)
          {
            params.vals.stage_slew &= ~(1 << stage_selected);
          }
          else if(v > 0)
          {
            params.vals.stage_slew |= (1 << stage_selected);
          }
          seq_values.stage_slew[stage_selected] = (bool)((params.vals.stage_slew >> stage_selected) & 1);
          display->fill_rect(16 * stage_selected + 1, 59, 14, 4, DUINO_SSD1306::Black);
          display->fill_rect(16 * stage_selected + 2 + 6 * (~(params.vals.stage_slew >> stage_selected) & 1), 60, 6, 2,
              DUINO_SSD1306::White);  
          break;
      }

      // mark save box
      if(main_selected)
      {
        display->fill_rect(123, 2, 3, 3, DUINO_SSD1306::Black);
      }

      display_changed = (bool)main_selected;
    }

    // display gate
    if(gate != last_gate || stage != last_stage)
    {
      if(gate)
      {
        if(stage != last_stage)
        {
          clear_gate();
        }
        display_gate(stage);
      }
      else
      {
        clear_gate();
      }

      last_gate = gate;
      last_stage = stage;
      gate_changed = true;
    }

    if(display_changed)
    {
      display->display_all();
      display_changed = false;
    }
    else if(gate_changed)
    {
      display->display(0, 127, 1, 1);
      gate_changed = false;
    }
  }

 private:
  void update_clock()
  {
    Timer1.detachInterrupt();
    if(!seq_values.clock_ext)
    {
      Timer1.attachInterrupt(clock_isr, seq_values.clock_period);
    }
  }

  float note_to_cv(int8_t note)
  {
    return ((float)note - 36.0) / 12.0;
  }

  unsigned long bpm_to_us(uint8_t bpm)
  {
    return 750000 / (unsigned long)bpm;
  }

  void display_slew_rate(int16_t x, int16_t y, uint8_t rate, DUINO_SSD1306::SSD1306Color color)
  {
    display->draw_vline(x + rate - 1, y, 5, color);
  }

  void display_gate_time(int16_t x, int16_t y, uint8_t time, DUINO_SSD1306::SSD1306Color color)
  {
    if(time > 1)
    {
      display->draw_hline(x, y + 1, time - 1, color);
    }
    display->draw_vline(x + time - 1, y + 1, 3, color);
    if(time < 16)
    {
      display->draw_hline(x + time, y + 3, 16 - time, color);
    }
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
    display->draw_char(x + 9, y + 7, '0' + note / 12, color);

    // draw note
    display->draw_char(x + 2, y + 4, semitone_lt[note % 12], color);
    
    // draw intonation symbol
    switch(semitone_in[note % 12])
    {
      case IF:
        display->draw_vline(x + 9, y + 1, 5, color);
        display->draw_pixel(x + 10, y + 3, color);
        display->draw_pixel(x + 10, y + 5, color);
        display->draw_vline(x + 11, y + 4, 2, color);
        break;
      case IS:
        display->draw_vline(x + 9, y + 1, 5, color);
        display->draw_pixel(x + 10, y + 2, color);
        display->draw_pixel(x + 10, y + 4, color);
        display->draw_vline(x + 11, y + 1, 5, color);
        break;
    }
  }

  void display_steps(int16_t x, int16_t y, int8_t steps, DUINO_SSD1306::SSD1306Color color)
  {
    // draw center line
    display->draw_hline(x + 1, y + 3, 14, color);

    // draw top pips
    for(uint8_t i = 0; i < 4; ++i)
    {
      if(i + 1 > steps)
      {
        break;
      }
      display->fill_rect(x + 1 + i * 4, y + 1, 2, 2, color);
    }

    // draw bottom pips
    for(uint8_t i = 0; i < 4; ++i)
    {
      if(i + 5 > steps)
      {
        break;
      }
      display->fill_rect(x + 13 - i * 4, y + 4, 2, 2, color);
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
        display->fill_rect(16 * stage_selected, 20, 16, 15, DUINO_SSD1306::Inverse);
        break;
      case 3: // steps
        display->fill_rect(16 * stage_selected, 35, 16, 7, DUINO_SSD1306::Inverse);
        break;
      case 4: // gate
        display->fill_rect(16 * stage_selected, 42, 16, 16, DUINO_SSD1306::Inverse);
        break;
      case 5: // slew
        display->fill_rect(16 * stage_selected, 58, 16, 6, DUINO_SSD1306::Inverse);
        break;
    }
  }

  void display_gate(uint8_t stage)
  {
    display->fill_rect(16 * stage + 6, 12, 4, 4, DUINO_SSD1306::White);
  }

  void clear_gate()
  {
    display->fill_rect(6, 12, 116, 4, DUINO_SSD1306::Black);
  }

  uint8_t main_selected;  // 0 - save, 1 - top, 2 - pitch, 3 - steps, 4 - gate, 5 - slew
  uint8_t top_selected;   // 0 - count, 1 - dir/add, 2 - slew, 3 - gate, 4 - clock
  uint8_t stage_selected;

  bool last_gate;
  uint8_t last_stage;

  bool display_changed, gate_changed;

  struct DU_SEQ_Parameter_Values {
    int8_t stage_pitch[8];
    int8_t stage_steps[8];
    int8_t stage_gate[8];
    uint8_t stage_slew;
    int8_t stage_count;
    uint8_t diradd_mode;
    int8_t slew_rate;
    int8_t gate_time;
    int8_t clock_bpm;
  };

  union DU_SEQ_Parameters {
    DU_SEQ_Parameter_Values vals;
    uint8_t bytes[30];
  };

  DU_SEQ_Parameters params;
};

DU_SEQ_Function * function;
DU_SEQ_Interface * interface;

ENCODER_ISR(interface->encoder);

void clock_isr()
{
  clock_gate = seq_values.clock_ext ? function->gt_read_debounce(GT3) : !clock_gate;

  if(clock_gate)
  {
    step++;
    step %= seq_values.stage_steps[stage];
    if(!step)
    {
      stage++;
      stage %= seq_values.stage_count;
    }
    retrigger = true;
  }
}

void reset_isr()
{
  stage = step = 0;
}

void setup()
{
  function = new DU_SEQ_Function();
  interface = new DU_SEQ_Interface();

  function->begin();
  interface->begin();
}

void loop()
{
  function->loop();
  interface->loop();
}
