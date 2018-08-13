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
 * DU-INO Quantizer Function
 * Aaron Mavrinac <aaron@logick.ca>
 */

#include <du-ino_function.h>
#include <du-ino_scales.h>
#include <avr/pgmspace.h>

static const unsigned char icons[] PROGMEM = {
  0x60, 0x78, 0x78, 0x78, 0x78, 0x78, 0x60, // trigger mode off
  0x60, 0x63, 0x0f, 0x0f, 0x0f, 0x63, 0x60  // trigger mode on
};

volatile bool triggered;

void trig_isr();

class DU_Quantizer_Function : public DUINO_Function {
 public:
  DU_Quantizer_Function() : DUINO_Function(0b00001100) { }

  virtual void setup()
  {
    trigger_mode = false;
    key = 0;
    scale_id = 0;
    scale = get_scale_by_id(scale_id);
    output_octave = 0;
    output_note = 0;
    main_selected = false;
    top_selected = 2;
    note_selected = 0;
    current_displayed_note = 0;

    gt_attach_interrupt(GT3, trig_isr, FALLING);

    // draw top line
    display->draw_du_logo_sm(0, 0, DUINO_SH1106::White);
    display->draw_text(16, 0, "QNTZR", DUINO_SH1106::White);

    // draw C key
    display->draw_vline(1, 11, 52, DUINO_SH1106::White);
    display->draw_hline(2, 11, 9, DUINO_SH1106::White);
    display->draw_hline(2, 62, 15, DUINO_SH1106::White);
    display->draw_vline(11, 11, 29, DUINO_SH1106::White);
    display->draw_hline(12, 39, 5, DUINO_SH1106::White);
    display->draw_vline(17, 39, 24, DUINO_SH1106::White);

    // draw Db key
    display->draw_vline(13, 11, 27, DUINO_SH1106::White);
    display->draw_hline(14, 11, 9, DUINO_SH1106::White);
    display->draw_hline(14, 37, 9, DUINO_SH1106::White);
    display->draw_vline(23, 11, 27, DUINO_SH1106::White);

    // draw D key
    display->draw_vline(19, 39, 24, DUINO_SH1106::White);
    display->draw_hline(20, 39, 5, DUINO_SH1106::White);
    display->draw_hline(20, 62, 15, DUINO_SH1106::White);
    display->draw_vline(25, 11, 29, DUINO_SH1106::White);
    display->draw_hline(26, 11, 3, DUINO_SH1106::White);
    display->draw_vline(29, 11, 29, DUINO_SH1106::White);
    display->draw_hline(30, 39, 5, DUINO_SH1106::White);
    display->draw_vline(35, 39, 24, DUINO_SH1106::White);

    // draw Eb key
    display->draw_vline(31, 11, 27, DUINO_SH1106::White);
    display->draw_hline(32, 11, 9, DUINO_SH1106::White);
    display->draw_hline(32, 37, 9, DUINO_SH1106::White);
    display->draw_vline(41, 11, 27, DUINO_SH1106::White);

    // draw E key
    display->draw_vline(37, 39, 24, DUINO_SH1106::White);
    display->draw_hline(38, 39, 5, DUINO_SH1106::White);
    display->draw_hline(38, 62, 15, DUINO_SH1106::White);
    display->draw_vline(43, 11, 29, DUINO_SH1106::White);
    display->draw_hline(44, 11, 9, DUINO_SH1106::White);
    display->draw_vline(53, 11, 52, DUINO_SH1106::White);

    // draw F key
    display->draw_vline(55, 11, 52, DUINO_SH1106::White);
    display->draw_hline(56, 11, 9, DUINO_SH1106::White);
    display->draw_hline(56, 62, 15, DUINO_SH1106::White);
    display->draw_vline(65, 11, 29, DUINO_SH1106::White);
    display->draw_hline(66, 39, 5, DUINO_SH1106::White);
    display->draw_vline(71, 39, 24, DUINO_SH1106::White);

    // draw Gb key
    display->draw_vline(67, 11, 27, DUINO_SH1106::White);
    display->draw_hline(68, 11, 9, DUINO_SH1106::White);
    display->draw_hline(68, 37, 9, DUINO_SH1106::White);
    display->draw_vline(77, 11, 27, DUINO_SH1106::White);

    // draw G key
    display->draw_vline(73, 39, 24, DUINO_SH1106::White);
    display->draw_hline(74, 39, 5, DUINO_SH1106::White);
    display->draw_hline(74, 62, 15, DUINO_SH1106::White);
    display->draw_vline(79, 11, 29, DUINO_SH1106::White);
    display->draw_hline(80, 11, 3, DUINO_SH1106::White);
    display->draw_vline(83, 11, 29, DUINO_SH1106::White);
    display->draw_hline(84, 39, 5, DUINO_SH1106::White);
    display->draw_vline(89, 39, 24, DUINO_SH1106::White);

    // draw Ab key
    display->draw_vline(85, 11, 27, DUINO_SH1106::White);
    display->draw_hline(86, 11, 9, DUINO_SH1106::White);
    display->draw_hline(86, 37, 9, DUINO_SH1106::White);
    display->draw_vline(95, 11, 27, DUINO_SH1106::White);

    // draw A key
    display->draw_vline(91, 39, 24, DUINO_SH1106::White);
    display->draw_hline(92, 39, 5, DUINO_SH1106::White);
    display->draw_hline(92, 62, 15, DUINO_SH1106::White);
    display->draw_vline(97, 11, 29, DUINO_SH1106::White);
    display->draw_hline(98, 11, 3, DUINO_SH1106::White);
    display->draw_vline(101, 11, 29, DUINO_SH1106::White);
    display->draw_hline(102, 39, 5, DUINO_SH1106::White);
    display->draw_vline(107, 39, 24, DUINO_SH1106::White);

    // draw Bb key
    display->draw_vline(103, 11, 27, DUINO_SH1106::White);
    display->draw_hline(104, 11, 9, DUINO_SH1106::White);
    display->draw_hline(104, 37, 9, DUINO_SH1106::White);
    display->draw_vline(113, 11, 27, DUINO_SH1106::White);

    // draw B key
    display->draw_vline(109, 39, 24, DUINO_SH1106::White);
    display->draw_hline(110, 39, 5, DUINO_SH1106::White);
    display->draw_hline(110, 62, 15, DUINO_SH1106::White);
    display->draw_vline(115, 11, 29, DUINO_SH1106::White);
    display->draw_hline(116, 11, 9, DUINO_SH1106::White);
    display->draw_vline(125, 11, 52, DUINO_SH1106::White);

    invert_note(current_displayed_note);
    invert_current_selection();
    display->display_all();

    display_trigger_mode();
    display_key();
    display_scale();
  }

  virtual void loop()
  {
    if(!trigger_mode || triggered)
    {
      if(!scale)
      {
        cv_out(CO1, 0.0);
        output_octave = 0;
        output_note = 0;
        triggered = false;
        return;
      }

      // read input
      float input = cv_read(CI1);

      // find a lower bound
      int below_octave = (int)input - 1;
      uint8_t below_note = 0;
      while(!(scale & (1 << below_note)))
      {
        below_note++;
      }

      // find closest lower and upper values
      int above_octave = below_octave, octave = below_octave;
      uint8_t above_note = below_note, note = below_note;
      while(note_cv(above_octave, key, above_note) < input)
      {
        note++;
        if(note > 11)
        {
          note = 0;
          octave++;
        }

        if(scale & (1 << note))
        {
          below_octave = above_octave;
          below_note = above_note;
          above_octave = octave;
          above_note = note;
        }
      }

      // output the nearer of the two values
      float below = note_cv(below_octave, key, below_note);
      float above = note_cv(above_octave, key, above_note);
      if(input - below < above - input)
      {
        cv_out(CO1, below);
        octave = below_octave;
        note = below_note;
      }
      else
      {
        cv_out(CO1, above);
        octave = above_octave;
        note = above_note;
      }

      // send trigger and update display note if note has changed
      if(octave != output_octave || note != output_note)
      {
        gt_out(GT1, true, true);
        output_octave = octave;
        output_note = note;
      }

      // reset trigger
      triggered = false;
    }

    // handle encoder button press
    DUINO_Encoder::Button b = encoder->get_button();
    if(b == DUINO_Encoder::DoubleClicked)
    {
      invert_current_selection();
      main_selected = !main_selected;
      invert_current_selection();
    }
    else if(b == DUINO_Encoder::Clicked)
    {
      if(main_selected)
      {
        scale ^= (1 << note_selected);
        scale_id = get_id_from_scale(scale);
        display_scale();
        triggered = true;
      }
      else
      {
        invert_current_selection();
        top_selected++;
        top_selected %= 3;
        invert_current_selection();
      }
    }

    // handle encoder spin
    int16_t v = encoder->get_value();
    if(v)
    {
      if(main_selected)
      {
        invert_current_selection();
        note_selected += v;
        note_selected %= 12;
        if(note_selected < 0)
        {
          note_selected += 12;
        }
        invert_current_selection();
      }
      else
      {
        switch(top_selected)
        {
          case 0: // trigger mode
            trigger_mode = v > 0;
            display_trigger_mode();
            break;
          case 1: // key
            key += v;
            key %= 12;
            if(key < 0)
            {
              key += 12;
            }
            display_key();
            triggered = true;
            break;
          case 2: // scale
            scale_id += v;
            if(scale_id < -1)
            {
              scale_id = -1;
            }
            else if(scale_id >= N_SCALES)
            {
              scale_id = N_SCALES - 1;
            }
            scale = get_scale_by_id(scale_id);
            display_scale();
            triggered = true;
            break;
        }
      }
    }

    // display current note
    if(output_note != current_displayed_note)
    {
      invert_note(current_displayed_note);
      invert_note(output_note);
      current_displayed_note = output_note;
    }
  }

 private:
  float note_cv(int octave, uint8_t key, uint8_t note)
  {
    return (float)octave + (float)key / 12.0 + (float)note / 12.0;
  }

  void get_note_indicator_coordinates(uint8_t note, uint16_t * x, uint16_t * y)
  {
    switch(note)
    {
      case 0: // C
        *x = 6;
        *y = 54;
        break;
      case 1: // Db
        *x = 15;
        *y = 29;
        break;
      case 2: // D
        *x = 24;
        *y = 54;
        break;
      case 3: // Eb
        *x = 33;
        *y = 29;
        break;
      case 4: // E
        *x = 42;
        *y = 54;
        break;
      case 5: // F
        *x = 60;
        *y = 54;
        break;
      case 6: // Gb
        *x = 69;
        *y = 29;
        break;
      case 7: // G
        *x = 78;
        *y = 54;
        break;
      case 8: // Ab
        *x = 87;
        *y = 29;
        break;
      case 9: // A
        *x = 96;
        *y = 54;
        break;
      case 10: // Bb
        *x = 105;
        *y = 29;
        break;
      case 11: // B
        *x = 114;
        *y = 54;
        break;
    }
  }

  void invert_current_selection()
  {
    if(main_selected)
    {
      int16_t x, y;
      get_note_indicator_coordinates(note_selected, &x, &y);
      display->draw_pixel(x, y, DUINO_SH1106::Inverse);
      display->draw_pixel(x + 2, y, DUINO_SH1106::Inverse);
      display->draw_pixel(x + 4, y, DUINO_SH1106::Inverse);
      display->draw_pixel(x + 6, y, DUINO_SH1106::Inverse);
      display->draw_pixel(x, y + 2, DUINO_SH1106::Inverse);
      display->draw_pixel(x, y + 4, DUINO_SH1106::Inverse);
      display->draw_pixel(x, y + 6, DUINO_SH1106::Inverse);
      display->draw_pixel(x + 2, y + 6, DUINO_SH1106::Inverse);
      display->draw_pixel(x + 4, y + 6, DUINO_SH1106::Inverse);
      display->draw_pixel(x + 6, y + 6, DUINO_SH1106::Inverse);
      display->draw_pixel(x + 6, y + 2, DUINO_SH1106::Inverse);
      display->draw_pixel(x + 6, y + 4, DUINO_SH1106::Inverse);
      if(y == 29)
      {
        display->display(x, x + 6, 3, 4);
      }
      else
      {
        display->display(x, x + 6, 6, 7);
      }
    }
    else
    {
      switch(top_selected)
      {
        case 0: // trigger mode
          display->fill_rect(76, 0, 9, 9, DUINO_SH1106::Inverse);
          display->display(76, 84, 0, 1);
          break;
        case 1: // key
          display->fill_rect(90, 0, 13, 9, DUINO_SH1106::Inverse);
          display->display(90, 102, 0, 1);
          break;
        case 2: // scale
          display->fill_rect(108, 0, 19, 9, DUINO_SH1106::Inverse);
          display->display(108, 126, 0, 1);
          break;
      }
    }
  }

  void display_trigger_mode()
  {
    bool selected = !main_selected && top_selected == 0;
    display->fill_rect(77, 1, 7, 7, selected ? DUINO_SH1106::White : DUINO_SH1106::Black);

    display->draw_bitmap_7(77, 1, icons, trigger_mode, selected ? DUINO_SH1106::Black : DUINO_SH1106::White);

    display->display(77, 83, 0, 0);
  }

  void display_key()
  {
    bool selected = !main_selected && top_selected == 1;
    display->fill_rect(91, 1, 11, 7, selected ? DUINO_SH1106::White : DUINO_SH1106::Black);

    bool sharp = false;
    unsigned char letter;
    switch(key)
    {
      case 1: // C#
        sharp = true;
      case 0: // C
        letter = 'C';
        break;
      case 3: // D#
        sharp = true;
      case 2: // D
        letter = 'D';
        break;
      case 4: // E
        letter = 'E';
        break;
      case 6: // F#
        sharp = true;
      case 5: // F
        letter = 'F';
        break;
      case 8: // G#
        sharp = true;
      case 7: // G
        letter = 'G';
        break;
      case 10: // A#
        sharp = true;
      case 9: // A
        letter = 'A';
        break;
      case 11: // B
        letter = 'B';
        break;
    }

    display->draw_char(91, 1, letter, selected ? DUINO_SH1106::Black : DUINO_SH1106::White);
    if(sharp)
    {
      display->draw_char(97, 1, '#', selected ? DUINO_SH1106::Black : DUINO_SH1106::White);
    }

    display->display(91, 101, 0, 0);
  }

  void display_scale()
  {
    bool selected = !main_selected && top_selected == 2;
    display->fill_rect(108, 0, 19, 9, selected ? DUINO_SH1106::White : DUINO_SH1106::Black);
    if(scale_id > -1)
    {
      for(uint8_t i = 0; i < 3; ++i)
      {
        display->draw_char(109 + i * 6, 1, pgm_read_byte(&scales[scale_id * 5 + 2 + i]),
            selected ? DUINO_SH1106::Black : DUINO_SH1106::White);
      }
    }

    int16_t x, y;
    for(uint8_t note = 0; note < 12; ++note)
    {  
      get_note_indicator_coordinates(note, &x, &y);
      bool white = scale & (1 << note);
      if(note == current_displayed_note)
      {
        white = !white;
      }
      display->fill_rect(x + 1, y + 1, 5, 5, white ? DUINO_SH1106::White : DUINO_SH1106::Black);
    }

    display->display_all();
  }

  void invert_note(uint8_t note)
  {
    bool refresh_lower = false;
    switch(note)
    {
      case 0:
        display->fill_rect(2, 12, 9, 50, DUINO_SH1106::Inverse);
        display->fill_rect(11, 40, 6, 22, DUINO_SH1106::Inverse);
        refresh_lower = true;
        break;
      case 1:
        display->fill_rect(14, 12, 9, 25, DUINO_SH1106::Inverse);
        break;
      case 2:
        display->fill_rect(26, 12, 3, 28, DUINO_SH1106::Inverse);
        display->fill_rect(20, 40, 15, 22, DUINO_SH1106::Inverse);
        refresh_lower = true;
        break;
      case 3:
        display->fill_rect(32, 12, 9, 25, DUINO_SH1106::Inverse);
        break;
      case 4:
        display->fill_rect(44, 12, 9, 50, DUINO_SH1106::Inverse);
        display->fill_rect(38, 40, 6, 22, DUINO_SH1106::Inverse);
        refresh_lower = true;
        break;
      case 5:
        display->fill_rect(56, 12, 9, 50, DUINO_SH1106::Inverse);
        display->fill_rect(65, 40, 6, 22, DUINO_SH1106::Inverse);
        refresh_lower = true;
        break;
      case 6:
        display->fill_rect(68, 12, 9, 25, DUINO_SH1106::Inverse);
        break;
      case 7:
        display->fill_rect(80, 12, 3, 28, DUINO_SH1106::Inverse);
        display->fill_rect(74, 40, 15, 22, DUINO_SH1106::Inverse);
        refresh_lower = true;
        break;
      case 8:
        display->fill_rect(86, 12, 9, 25, DUINO_SH1106::Inverse);
        break;
      case 9:
        display->fill_rect(98, 12, 3, 28, DUINO_SH1106::Inverse);
        display->fill_rect(92, 40, 15, 22, DUINO_SH1106::Inverse);
        refresh_lower = true;
        break;
      case 10:
        display->fill_rect(104, 12, 9, 25, DUINO_SH1106::Inverse);
        break;
      case 11:
        display->fill_rect(116, 12, 9, 50, DUINO_SH1106::Inverse);
        display->fill_rect(110, 40, 6, 22, DUINO_SH1106::Inverse);
        refresh_lower = true;
        break;
    }

    display->display(2, 124, 1, refresh_lower ? 7 : 4);
  }

  bool trigger_mode;
  int8_t key;
  int scale_id;
  uint16_t scale;
  uint8_t output_note;
  int output_octave;
  bool main_selected;
  uint8_t top_selected;  // 0 - trigger mode, 1 - key, 2 - scale
  int8_t note_selected;
  uint8_t current_displayed_note;
};

DU_Quantizer_Function * function;

ENCODER_ISR(function->encoder);

void trig_isr()
{
  triggered = true;
}

void setup()
{
  function = new DU_Quantizer_Function();

  triggered = false;

  function->begin();
}

void loop()
{
  function->loop();
}
