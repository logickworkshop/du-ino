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
 * DU-INO Arduino Library - SSD1306 Display Module
 * Aaron Mavrinac <aaron@logick.ca>
 */

/* This module is based in part upon the SSD1306 [1] and GFX [2] Arduino
 * libraries from Adafruit.
 * 
 * [1]: https://github.com/adafruit/Adafruit_SSD1306
 * [2]: https://github.com/adafruit/Adafruit-GFX-Library
 */

#ifndef DUINO_SSD1306_H_
#define DUINO_SSD1306_H_

#include "Arduino.h"

#define SSD1306_LCDWIDTH                                 128
#define SSD1306_LCDHEIGHT                 	              64

#define SSD1306_I2C_ADDRESS                             0x3D

#define SSD1306_SETCONTRAST                             0x81
#define SSD1306_DISPLAYALLON_RESUME                     0xA4
#define SSD1306_DISPLAYALLON                            0xA5
#define SSD1306_NORMALDISPLAY                           0xA6
#define SSD1306_INVERTDISPLAY                           0xA7
#define SSD1306_DISPLAYOFF                              0xAE
#define SSD1306_DISPLAYON                               0xAF

#define SSD1306_SETDISPLAYOFFSET                        0xD3
#define SSD1306_SETCOMPINS                              0xDA

#define SSD1306_SETVCOMDETECT                           0xDB

#define SSD1306_SETDISPLAYCLOCKDIV                      0xD5
#define SSD1306_SETPRECHARGE                            0xD9

#define SSD1306_SETMULTIPLEX                            0xA8

#define SSD1306_SETLOWCOLUMN                            0x00
#define SSD1306_SETHIGHCOLUMN                           0x10

#define SSD1306_SETSTARTLINE                            0x40

#define SSD1306_MEMORYMODE                              0x20
#define SSD1306_COLUMNADDR                              0x21
#define SSD1306_PAGEADDR                                0x22

#define SSD1306_COMSCANINC                              0xC0
#define SSD1306_COMSCANDEC                              0xC8

#define SSD1306_SEGREMAP                                0xA0

#define SSD1306_CHARGEPUMP                              0x8D

#define SSD1306_ACTIVATE_SCROLL                         0x2F
#define SSD1306_DEACTIVATE_SCROLL                       0x2E
#define SSD1306_SET_VERTICAL_SCROLL_AREA                0xA3
#define SSD1306_RIGHT_HORIZONTAL_SCROLL                 0x26
#define SSD1306_LEFT_HORIZONTAL_SCROLL                  0x27
#define SSD1306_VERTICAL_AND_RIGHT_HORIZONTAL_SCROLL    0x29
#define SSD1306_VERTICAL_AND_LEFT_HORIZONTAL_SCROLL     0x2A

class DUINO_SSD1306 
{
 public:
  enum SSD1306Color
  {
    Black,
    White,
    Inverse
  };

 public:
  DUINO_SSD1306();

  void ssd1306_command(uint8_t c);
  void begin();

  void display(uint8_t col_start, uint8_t col_end, uint8_t page_start, uint8_t page_end);
  void display_all();

  void clear_display();

  void draw_pixel(int16_t x, int16_t y, SSD1306Color color);
  void draw_hline(int16_t x, int16_t y, int16_t w, SSD1306Color color);
  void draw_vline(int16_t x, int16_t y, int16_t h, SSD1306Color color);
  void draw_circle(int16_t x, int16_t y, int16_t r, SSD1306Color color);
  void fill_circle(int16_t x, int16_t y, int16_t r, SSD1306Color color);
  void fill_rect(int16_t x, int16_t y, int16_t w, int16_t h, SSD1306Color color);
  void fill_screen(SSD1306Color color);

  void draw_char(int16_t x, int16_t y, unsigned char c, SSD1306Color color);
  void draw_text(int16_t x, int16_t y, const char * text, SSD1306Color color);

  void draw_icon_16(int16_t x, int16_t y, const unsigned char * map, unsigned char c, SSD1306Color color);

  void draw_du_logo_lg(int16_t x, int16_t y, SSD1306Color color);
  void draw_du_logo_sm(int16_t x, int16_t y, SSD1306Color color);
  void draw_logick_logo(int16_t x, int16_t y, SSD1306Color color);

 private:
  inline void draw_quadrants(int16_t xc, int16_t yc, int16_t x, int16_t y, SSD1306Color color);
  inline void fill_quadrants(int16_t xc, int16_t yc, int16_t x, int16_t y, SSD1306Color color);
};

#endif // DUINO_SSD1306_H_
