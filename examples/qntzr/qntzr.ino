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
 *
 * JACK    FUNCTION
 * ----    --------
 * GT1 O - note change trigger out
 * GT2 O -
 * GT3 I - note change trigger in
 * GT4 I -
 * CI1   - CV in
 * CI2   -
 * CI3   -
 * CI4   -
 * OFFST -
 * CO1   - quantized CV out
 * CO2   -
 * CO3   -
 * CO4   -
 * FNCTN -
 *
 * SWITCH CONFIGURATION
 * --------------------
 * SG2    [_][_]    SG1
 * SG4    [^][^]    SG3
 * SC2    [_][_]    SC1
 * SC4    [_][_]    SC3
 */

#include <du-ino_function.h>
#include <du-ino_widgets.h>
#include <du-ino_scales.h>
#include <avr/pgmspace.h>

static const unsigned char icons[] PROGMEM =
{
  0x60, 0x78, 0x78, 0x78, 0x78, 0x78, 0x60, // trigger mode off
  0x60, 0x63, 0x0f, 0x0f, 0x0f, 0x63, 0x60  // trigger mode on
};

void trig_isr();

void trigger_mode_scroll_callback(int delta);
void key_scroll_callback(int delta);
void scale_scroll_callback(int delta);
void notes_click_callback(uint8_t selected);

class DU_Quantizer_Function : public DUINO_Function
{
public:
  DU_Quantizer_Function() : DUINO_Function(0b00001100) { }

  virtual void function_setup()
  {
    // build widget hierarchy
    container_outer_ = new DUINO_WidgetContainer<2>(DUINO_Widget::DoubleClick);
    container_top_ = new DUINO_WidgetContainer<3>(DUINO_Widget::Click, 2);
    container_outer_->attach_child(container_top_, 0);
    widget_trigger_mode_ = new DUINO_DisplayWidget(76, 0, 9, 9, DUINO_Widget::Full);
    widget_trigger_mode_->attach_scroll_callback(trigger_mode_scroll_callback);
    container_top_->attach_child(widget_trigger_mode_, 0);
    widget_key_ = new DUINO_DisplayWidget(90, 0, 13, 9, DUINO_Widget::Full);
    widget_key_->attach_scroll_callback(key_scroll_callback);
    container_top_->attach_child(widget_key_, 1);
    widget_scale_ = new DUINO_DisplayWidget(108, 0, 19, 9, DUINO_Widget::Full);
    widget_scale_->attach_scroll_callback(scale_scroll_callback);
    container_top_->attach_child(widget_scale_, 2);
    container_notes_ = new DUINO_WidgetContainer<12>(DUINO_Widget::Scroll);
    container_outer_->attach_child(container_notes_, 1);
    widgets_notes_[0] = new DUINO_DisplayWidget(6, 54, 7, 7, DUINO_Widget::DottedBox);
    widgets_notes_[1] = new DUINO_DisplayWidget(15, 29, 7, 7, DUINO_Widget::DottedBox);
    widgets_notes_[2] = new DUINO_DisplayWidget(24, 54, 7, 7, DUINO_Widget::DottedBox);
    widgets_notes_[3] = new DUINO_DisplayWidget(33, 29, 7, 7, DUINO_Widget::DottedBox);
    widgets_notes_[4] = new DUINO_DisplayWidget(42, 54, 7, 7, DUINO_Widget::DottedBox);
    widgets_notes_[5] = new DUINO_DisplayWidget(60, 54, 7, 7, DUINO_Widget::DottedBox);
    widgets_notes_[6] = new DUINO_DisplayWidget(69, 29, 7, 7, DUINO_Widget::DottedBox);
    widgets_notes_[7] = new DUINO_DisplayWidget(78, 54, 7, 7, DUINO_Widget::DottedBox);
    widgets_notes_[8] = new DUINO_DisplayWidget(87, 29, 7, 7, DUINO_Widget::DottedBox);
    widgets_notes_[9] = new DUINO_DisplayWidget(96, 54, 7, 7, DUINO_Widget::DottedBox);
    widgets_notes_[10] = new DUINO_DisplayWidget(105, 29, 7, 7, DUINO_Widget::DottedBox);
    widgets_notes_[11] = new DUINO_DisplayWidget(114, 54, 7, 7, DUINO_Widget::DottedBox);
    for (uint8_t i = 0; i < 12; ++i)
    {
      container_notes_->attach_child(widgets_notes_[i], i);
    }
    container_notes_->attach_click_callback_array(notes_click_callback);

    triggered_ = false;
    trigger_mode_ = false;
    key_ = 0;
    scale_id_ = 0;
    scale_ = get_scale_by_id(scale_id_);
    output_octave_ = 0;
    output_note_ = 0;
    current_displayed_note_ = 0;

    gt_attach_interrupt(GT3, trig_isr, FALLING);

    // draw top line
    Display.draw_du_logo_sm(0, 0, DUINO_SH1106::White);
    Display.draw_text(16, 0, "QNTZR", DUINO_SH1106::White);

    // draw kays
    draw_left_key(1, 11);     // C
    draw_black_key(13, 11);   // Db
    draw_center_key(19, 11);  // D
    draw_black_key(31, 11);   // Eb
    draw_right_key(37, 11);   // E
    draw_left_key(55, 11);    // F
    draw_black_key(67, 11);   // Gb
    draw_center_key(73, 11);  // G
    draw_black_key(85, 11);   // Ab
    draw_center_key(91, 11);  // A
    draw_black_key(103, 11);  // Bb
    draw_right_key(109, 11);  // B

    invert_note(current_displayed_note_);
    widget_setup(container_outer_);
    Display.display();

    display_trigger_mode();
    display_key();
    display_scale();
  }

  virtual void function_loop()
  {
    if (!trigger_mode_ || triggered_)
    {
      if (scale_ == 0)
      {
        cv_out(CO1, 0.0);
        output_octave_ = 0;
        output_note_ = 0;
      }
      else
      {
        // read input
        float input = cv_read(CI1);

        // find a lower bound
        int below_octave = (int)input - 1;
        uint8_t below_note = 0;
        while (!(scale_ & (1 << below_note)))
        {
          below_note++;
        }

        // find closest lower and upper values
        int above_octave = below_octave, octave = below_octave;
        uint8_t above_note = below_note, note = below_note;
        while (note_cv(above_octave, key_, above_note) < input)
        {
          note++;
          if (note > 11)
          {
            note = 0;
            octave++;
          }

          if (scale_ & (1 << note))
          {
            below_octave = above_octave;
            below_note = above_note;
            above_octave = octave;
            above_note = note;
          }
        }

        // output the nearer of the two values
        float below = note_cv(below_octave, key_, below_note);
        float above = note_cv(above_octave, key_, above_note);
        if (input - below < above - input)
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
        if (octave != output_octave_ || note != output_note_)
        {
          gt_out(GT1, true, true);
          output_octave_ = octave;
          output_note_ = note;
        }
      }
      
      // reset trigger
      triggered_ = false;
    }

    widget_loop();

    // display current note
    if (output_note_ != current_displayed_note_)
    {
      invert_note(current_displayed_note_);
      invert_note(output_note_);
      current_displayed_note_ = output_note_;
    }
  }

  void trig_callback()
  {
    triggered_ = true;
  }

  void widget_trigger_mode_scroll_callback(int delta)
  {
    trigger_mode_ = delta > 0;
    display_trigger_mode();
  }

  void widget_key_scroll_callback(int delta)
  {
    key_ += delta;
    key_ %= 12;
    if(key_ < 0)
    {
      key_ += 12;
    }
    display_key();
    triggered_ = true;
  }

  void widget_scale_scroll_callback(int delta)
  {
    scale_id_ += delta;
    if (scale_id_ < -1)
    {
      scale_id_ = -1;
    }
    else if (scale_id_ >= N_SCALES)
    {
      scale_id_ = N_SCALES - 1;
    }
    scale_ = get_scale_by_id(scale_id_);
    display_scale();
    triggered_ = true;
  }

  void widgets_notes_click_callback(uint8_t selected)
  {
    scale_ ^= (1 << selected);
    scale_id_ = get_id_from_scale(scale_);
    display_scale();
    triggered_ = true;
  }

private:
  float note_cv(int octave, uint8_t key_, uint8_t note)
  {
    return (float)octave + (float)key_ / 12.0 + (float)note / 12.0;
  }

  void draw_left_key(int16_t x, int16_t y)
  {
    Display.draw_vline(x, y, 52, DUINO_SH1106::White);
    Display.draw_hline(x + 1, y, 9, DUINO_SH1106::White);
    Display.draw_hline(x + 1, y + 51, 15, DUINO_SH1106::White);
    Display.draw_vline(x + 10, y, 29, DUINO_SH1106::White);
    Display.draw_hline(x + 11, y + 28, 5, DUINO_SH1106::White);
    Display.draw_vline(x + 16, y + 28, 24, DUINO_SH1106::White);
  }

  void draw_center_key(int16_t x, int16_t y)
  {
    Display.draw_vline(x, y + 28, 24, DUINO_SH1106::White);
    Display.draw_hline(x + 1, y + 28, 5, DUINO_SH1106::White);
    Display.draw_hline(x + 1, y + 51, 15, DUINO_SH1106::White);
    Display.draw_vline(x + 6, y, 29, DUINO_SH1106::White);
    Display.draw_hline(x + 7, y, 3, DUINO_SH1106::White);
    Display.draw_vline(x + 10, y, 29, DUINO_SH1106::White);
    Display.draw_hline(x + 11, y + 28, 5, DUINO_SH1106::White);
    Display.draw_vline(x + 16, y + 28, 24, DUINO_SH1106::White);
  }

  void draw_right_key(int16_t x, int16_t y)
  {
    Display.draw_vline(x, y + 28, 24, DUINO_SH1106::White);
    Display.draw_hline(x + 1, y + 28, 5, DUINO_SH1106::White);
    Display.draw_hline(x + 1, y + 51, 15, DUINO_SH1106::White);
    Display.draw_vline(x + 6, y, 29, DUINO_SH1106::White);
    Display.draw_hline(x + 7, y, 9, DUINO_SH1106::White);
    Display.draw_vline(x + 16, y, 52, DUINO_SH1106::White);
  }

  void draw_black_key(int16_t x, int16_t y)
  {
    Display.draw_vline(x, y, 27, DUINO_SH1106::White);
    Display.draw_hline(x + 1, y, 9, DUINO_SH1106::White);
    Display.draw_hline(x + 1, y + 26, 9, DUINO_SH1106::White);
    Display.draw_vline(x + 10, y, 27, DUINO_SH1106::White);
  }

  void display_trigger_mode()
  {
    Display.fill_rect(widget_trigger_mode_->x() + 1, widget_trigger_mode_->y() + 1, 7, 7,
        widget_trigger_mode_->inverted() ? DUINO_SH1106::White : DUINO_SH1106::Black);
    Display.draw_bitmap_7(widget_trigger_mode_->x() + 1, widget_trigger_mode_->y() + 1, icons, trigger_mode_,
        widget_trigger_mode_->inverted() ? DUINO_SH1106::Black : DUINO_SH1106::White);
    widget_trigger_mode_->display();
  }

  void display_key()
  {
    Display.fill_rect(widget_key_->x() + 1, widget_key_->y() + 1, 11, 7,
        widget_key_->inverted() ? DUINO_SH1106::White : DUINO_SH1106::Black);

    bool sharp = false;
    unsigned char letter;
    switch (key_)
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

    Display.draw_char(widget_key_->x() + 1, widget_key_->y() + 1, letter,
        widget_key_->inverted() ? DUINO_SH1106::Black : DUINO_SH1106::White);
    if (sharp)
    {
      Display.draw_char(widget_key_->x() + 7, widget_key_->y() + 1, '#',
          widget_key_->inverted() ? DUINO_SH1106::Black : DUINO_SH1106::White);
    }

    widget_key_->display();
  }

  void display_scale()
  {
    Display.fill_rect(widget_scale_->x() + 1, widget_scale_->y() + 1, 17, 7,
        widget_scale_->inverted() ? DUINO_SH1106::White : DUINO_SH1106::Black);
    if (scale_id_ > -1)
    {
      for (uint8_t i = 0; i < 3; ++i)
      {
        Display.draw_char(widget_scale_->x() + 1 + i * 6, widget_scale_->y() + 1,
            pgm_read_byte(&scales[scale_id_ * 5 + 2 + i]),
            widget_scale_->inverted() ? DUINO_SH1106::Black : DUINO_SH1106::White);
      }
    }

    for (uint8_t note = 0; note < 12; ++note)
    {  
      bool white = scale_ & (1 << note);
      if (note == current_displayed_note_)
      {
        white = !white;
      }
      Display.fill_rect(widgets_notes_[note]->x() + 1, widgets_notes_[note]->y() + 1, 5, 5,
          white ? DUINO_SH1106::White : DUINO_SH1106::Black);
    }

    Display.display();
  }

  void invert_note(uint8_t note)
  {
    bool refresh_lower = false;
    switch (note)
    {
      case 0:
        Display.fill_rect(2, 12, 9, 50, DUINO_SH1106::Inverse);
        Display.fill_rect(11, 40, 6, 22, DUINO_SH1106::Inverse);
        refresh_lower = true;
        break;
      case 1:
        Display.fill_rect(14, 12, 9, 25, DUINO_SH1106::Inverse);
        break;
      case 2:
        Display.fill_rect(26, 12, 3, 28, DUINO_SH1106::Inverse);
        Display.fill_rect(20, 40, 15, 22, DUINO_SH1106::Inverse);
        refresh_lower = true;
        break;
      case 3:
        Display.fill_rect(32, 12, 9, 25, DUINO_SH1106::Inverse);
        break;
      case 4:
        Display.fill_rect(44, 12, 9, 50, DUINO_SH1106::Inverse);
        Display.fill_rect(38, 40, 6, 22, DUINO_SH1106::Inverse);
        refresh_lower = true;
        break;
      case 5:
        Display.fill_rect(56, 12, 9, 50, DUINO_SH1106::Inverse);
        Display.fill_rect(65, 40, 6, 22, DUINO_SH1106::Inverse);
        refresh_lower = true;
        break;
      case 6:
        Display.fill_rect(68, 12, 9, 25, DUINO_SH1106::Inverse);
        break;
      case 7:
        Display.fill_rect(80, 12, 3, 28, DUINO_SH1106::Inverse);
        Display.fill_rect(74, 40, 15, 22, DUINO_SH1106::Inverse);
        refresh_lower = true;
        break;
      case 8:
        Display.fill_rect(86, 12, 9, 25, DUINO_SH1106::Inverse);
        break;
      case 9:
        Display.fill_rect(98, 12, 3, 28, DUINO_SH1106::Inverse);
        Display.fill_rect(92, 40, 15, 22, DUINO_SH1106::Inverse);
        refresh_lower = true;
        break;
      case 10:
        Display.fill_rect(104, 12, 9, 25, DUINO_SH1106::Inverse);
        break;
      case 11:
        Display.fill_rect(116, 12, 9, 50, DUINO_SH1106::Inverse);
        Display.fill_rect(110, 40, 6, 22, DUINO_SH1106::Inverse);
        refresh_lower = true;
        break;
    }

    Display.display(2, 124, 1, refresh_lower ? 7 : 4);
  }

  DUINO_WidgetContainer<2> * container_outer_;
  DUINO_WidgetContainer<3> * container_top_;
  DUINO_WidgetContainer<12> * container_notes_;
  DUINO_DisplayWidget * widget_trigger_mode_;
  DUINO_DisplayWidget * widget_key_;
  DUINO_DisplayWidget * widget_scale_;
  DUINO_DisplayWidget * widgets_notes_[12];

  volatile bool triggered_;
  bool trigger_mode_;
  int8_t key_;
  int scale_id_;
  uint16_t scale_;
  uint8_t output_note_;
  int output_octave_;
  uint8_t current_displayed_note_;
};

DU_Quantizer_Function * function;

void trig_isr() { function->trig_callback(); }

void trigger_mode_scroll_callback(int delta) { function->widget_trigger_mode_scroll_callback(delta); }
void key_scroll_callback(int delta) { function->widget_key_scroll_callback(delta); }
void scale_scroll_callback(int delta) { function->widget_scale_scroll_callback(delta); }
void notes_click_callback(uint8_t selected) { function->widgets_notes_click_callback(selected); }

void setup()
{
  function = new DU_Quantizer_Function();

  function->begin();
}

void loop()
{
  function->function_loop();
}
