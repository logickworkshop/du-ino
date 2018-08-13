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
 * DU-INO Arduino Library - SH1106 Display Module
 * Aaron Mavrinac <aaron@logick.ca>
 */

/* This module is based in part upon the SSD1306 [1] and GFX [2] Arduino
 * libraries from Adafruit.
 * 
 * [1]: https://github.com/adafruit/Adafruit_SSD1306
 * [2]: https://github.com/adafruit/Adafruit-GFX-Library
 */

#ifndef DUINO_SH1106_H_
#define DUINO_SH1106_H_

#include "Arduino.h"

#define SH1106_LCDWIDTH                                  128
#define SH1106_LCDHEIGHT                 	               64

#define SH1106_SETLOWCOLUMN                             0x00
#define SH1106_SETHIGHCOLUMN                            0x10
#define SH1106_SETPUMPVOLTAGE                           0x30
#define SH1106_SETSTARTLINE                             0x40
#define SH1106_SETCONTRAST                              0x81
#define SH1106_SEGREMAP                                 0xA0
#define SH1106_DISPLAYALLON_RESUME                      0xA4
#define SH1106_DISPLAYALLON                             0xA5
#define SH1106_NORMALDISPLAY                            0xA6
#define SH1106_INVERTDISPLAY                            0xA7
#define SH1106_SETMULTIPLEX                             0xA8
#define SH1106_DISPLAYOFF                               0xAE
#define SH1106_DISPLAYON                                0xAF
#define SH1106_SETPAGEADDR                              0xB0
#define SH1106_COMSCANINC                               0xC0
#define SH1106_COMSCANDEC                               0xC8
#define SH1106_SETDISPLAYOFFSET                         0xD3
#define SH1106_SETDISPLAYCLOCKDIV                       0xD5
#define SH1106_SETPRECHARGE                             0xD9
#define SH1106_SETCOMPINS                               0xDA
#define SH1106_SETVCOMDETECT                            0xDB

class DUINO_SH1106 
{
public:
  enum SH1106Color
  {
    Black,
    White,
    Inverse
  };

  DUINO_SH1106(uint8_t ss, uint8_t dc);

  void sh1106_command(uint8_t command);
  void sh1106_spi_write(uint8_t data);
  void begin();

  void display(uint8_t col_start, uint8_t col_end, uint8_t page_start, uint8_t page_end);
  void display_all();

  void clear_display();

  void draw_pixel(int16_t x, int16_t y, SH1106Color color);
  void draw_hline(int16_t x, int16_t y, int16_t w, SH1106Color color);
  void draw_vline(int16_t x, int16_t y, int16_t h, SH1106Color color);
  void draw_circle(int16_t x, int16_t y, int16_t r, SH1106Color color);
  void fill_circle(int16_t x, int16_t y, int16_t r, SH1106Color color);
  void fill_rect(int16_t x, int16_t y, int16_t w, int16_t h, SH1106Color color);
  void fill_screen(SH1106Color color);

  void draw_char(int16_t x, int16_t y, unsigned char c, SH1106Color color);
  void draw_text(int16_t x, int16_t y, const char * text, SH1106Color color);

  void draw_bitmap_7(int16_t x, int16_t y, const unsigned char * map, unsigned char c, SH1106Color color);
  void draw_bitmap_8(int16_t x, int16_t y, const unsigned char * map, unsigned char c, SH1106Color color);

  void draw_du_logo_lg(int16_t x, int16_t y, SH1106Color color);
  void draw_du_logo_sm(int16_t x, int16_t y, SH1106Color color);
  void draw_logick_logo(int16_t x, int16_t y, SH1106Color color);

private:
  inline void draw_quadrants(int16_t xc, int16_t yc, int16_t x, int16_t y, SH1106Color color);
  inline void fill_quadrants(int16_t xc, int16_t yc, int16_t x, int16_t y, SH1106Color color);

  const uint8_t pin_ss_;
  const uint8_t pin_dc_;
};

#endif // DUINO_SH1106_H_
