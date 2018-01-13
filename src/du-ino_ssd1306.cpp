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

#include <stdlib.h>
#include <Wire.h>
#include <avr/pgmspace.h>

#include "du-ino_font5x7.h"
#include "du-ino_ssd1306.h"

static uint8_t buffer[SSD1306_LCDHEIGHT * SSD1306_LCDWIDTH / 8];

DUINO_SSD1306::DUINO_SSD1306()
{

}

void DUINO_SSD1306::ssd1306_command(uint8_t c)
{
  // send I2C command
  uint8_t control = 0x00;
  Wire.beginTransmission(SSD1306_I2C_ADDRESS);
  Wire.write(control);
  Wire.write(c);
  Wire.endTransmission();
}

void DUINO_SSD1306::begin()
{
  // initialize I2C
  Wire.begin();
#ifdef TWBR
  TWBR = 12;
#endif

  // initialization sequence
  ssd1306_command(SSD1306_DISPLAYOFF);

  ssd1306_command(SSD1306_SETDISPLAYCLOCKDIV);
  ssd1306_command(0x80);

  ssd1306_command(SSD1306_SETMULTIPLEX);
  ssd1306_command(SSD1306_LCDHEIGHT - 1);

  ssd1306_command(SSD1306_SETDISPLAYOFFSET);
  ssd1306_command(0x0);
  ssd1306_command(SSD1306_SETSTARTLINE | 0x0);
  ssd1306_command(SSD1306_CHARGEPUMP);
  ssd1306_command(0x14);
  ssd1306_command(SSD1306_MEMORYMODE);
  ssd1306_command(0x00);
  ssd1306_command(SSD1306_SEGREMAP | 0x1);
  ssd1306_command(SSD1306_COMSCANDEC);

  ssd1306_command(SSD1306_SETCOMPINS);
  ssd1306_command(0x12);
  ssd1306_command(SSD1306_SETCONTRAST);
  ssd1306_command(0xCF);

  ssd1306_command(SSD1306_SETPRECHARGE);
  ssd1306_command(0xF1);
  ssd1306_command(SSD1306_SETVCOMDETECT);
  ssd1306_command(0x40);
  ssd1306_command(SSD1306_DISPLAYALLON_RESUME);
  ssd1306_command(SSD1306_NORMALDISPLAY);

  ssd1306_command(SSD1306_DEACTIVATE_SCROLL);

  ssd1306_command(SSD1306_DISPLAYON);
}

void DUINO_SSD1306::display(uint8_t col_start, uint8_t col_end, uint8_t page_start, uint8_t page_end)
{
  // set column address range
  ssd1306_command(SSD1306_COLUMNADDR);
  ssd1306_command(col_start);
  ssd1306_command(col_end);

  // set page address range
  ssd1306_command(SSD1306_PAGEADDR);
  ssd1306_command(page_start);
  ssd1306_command(page_end);

  // transmit buffer
  uint8_t count = 0;
  Wire.beginTransmission(SSD1306_I2C_ADDRESS);
  Wire.write(0x40);
  for(uint8_t page = page_start; page <= page_end; ++page)
  {
    for(uint8_t col = col_start; col <= col_end; ++col)
    {
      Wire.write(buffer[page * 128 + col]);
      if(++count > 15)
      {
        count = 0;
        Wire.endTransmission();
        Wire.beginTransmission(SSD1306_I2C_ADDRESS);
        Wire.write(0x40);
      }
    }
  }
  Wire.endTransmission();
}

void DUINO_SSD1306::display_all()
{
  display(0, 127, 0, 7);
}

void DUINO_SSD1306::clear_display()
{
  // clear buffer
  memset(buffer, 0, SSD1306_LCDHEIGHT * SSD1306_LCDWIDTH / 8);
}

void DUINO_SSD1306::draw_pixel(int16_t x, int16_t y, SSD1306Color color)
{
  // bound check
  if ((x < 0) || (x >= SSD1306_LCDWIDTH) || (y < 0) || (y >= SSD1306_LCDHEIGHT))
    return;

  // set pixel value
  switch(color)
  {
    case Black:
      buffer[x + (y / 8) * SSD1306_LCDWIDTH] &= ~(1 << (y & 7));
      break;
    case White:
      buffer[x + (y / 8) * SSD1306_LCDWIDTH] |=  (1 << (y & 7));
      break;
    case Inverse:
      buffer[x + (y / 8) * SSD1306_LCDWIDTH] ^=  (1 << (y & 7));
      break;
  }
}

void DUINO_SSD1306::draw_hline(int16_t x, int16_t y, int16_t w, SSD1306Color color)
{
  // bound check
  if((y < 0) || (y >= SSD1306_LCDHEIGHT))
    return;

  // clip at edges of display
  if(x < 0)
  {
    w += x;
    x = 0;
  }
  if((x + w) > SSD1306_LCDWIDTH)
  {
    w = (SSD1306_LCDWIDTH - x);
  }

  // check width
  if(w <= 0)
    return;

  // initialize buffer pointer
  register uint8_t * bp = buffer + ((y / 8) * SSD1306_LCDWIDTH) + x;

  // initialize pixel mask
  register uint8_t mask = 1 << (y & 7);

  switch(color)
  {
    case Black:
      mask = ~mask;
      while(w--)
      {
        *bp++ &= mask;
      }
      break;
    case White:
      while(w--)
      {
        *bp++ |= mask;
      }
      break;
    case Inverse:
      while(w--)
      {
        *bp++ ^= mask;
      }
      break;
  }
}

void DUINO_SSD1306::draw_vline(int16_t x, int16_t y, int16_t h, SSD1306Color color)
{
  // bound check
  if((x < 0) || (x >= SSD1306_LCDWIDTH))
    return;

  // clip at edges of display
  if(y < 0)
  {
    h += y;
    y = 0;
  }
  if((y + h) > SSD1306_LCDHEIGHT)
  {
    h = (SSD1306_LCDHEIGHT - y);
  }

  // check height
  if(h <= 0)
    return;

  // use local byte registers for coordinates
  register uint8_t ry = y;
  register uint8_t rh = h;

  // set up the pointer for fast movement through the buffer
  register uint8_t *bp = buffer + ((ry / 8) * SSD1306_LCDWIDTH) + x;

  // first partial byte
  register uint8_t mod = (ry & 7);
  if(mod)
  {
    // mask high bits
    mod = 8 - mod;
    static uint8_t premask[8] = {0x00, 0x80, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC, 0xFE};
    register uint8_t mask = premask[mod];

    // adjust the mask
    if(rh < mod)
    {
      mask &= (0XFF >> (mod - rh));
    }

    switch(color)
    {
      case Black:
        *bp &= ~mask;
        break;
      case White:
        *bp |= mask;
        break;
      case Inverse:
        *bp ^= mask;
        break;
    }

    // exit if complete
    if(rh < mod)
      return;

    rh -= mod;
    bp += SSD1306_LCDWIDTH;
  }

  // solid bytes
  if(rh >= 8)
  {
    if(color == Inverse)
    {
      do
      {
        *bp = ~(*bp);
        bp += SSD1306_LCDWIDTH;
        rh -= 8;
      }
      while(rh >= 8);
    }
    else
    {
      register uint8_t val = (color == White) ? 0xFF : 0x00;

      do
      {
        *bp = val;
        bp += SSD1306_LCDWIDTH;
        rh -= 8;
      }
      while(rh >= 8);
    }
  }

  // last partial byte
  if(rh)
  {
    // mask low bits
    mod = rh & 7;
    static uint8_t postmask[8] = {0x00, 0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x7F};
    register uint8_t mask = postmask[mod];

    switch(color)
    {
      case Black:
        *bp &= ~mask;
        break;
      case White:
        *bp |= mask;
        break;
      case Inverse:
        *bp ^= mask;
        break;
    }
  }
}

void DUINO_SSD1306::draw_circle(int16_t xc, int16_t yc, int16_t r, SSD1306Color color)
{
  register int8_t x = 0;
  register int8_t y = r;
  register int8_t d = 3 - 2 * r;

  // Bresenham raster circle algorithm
  while(y >= x)
  {
    draw_quadrants(xc, yc, x, y, color);
    x++;

    if(d > 0)
    {
      y--; 
      d = d + 4 * (x - y) + 10;
    }
    else
    {
      d = d + 4 * x + 6;
    }
    draw_quadrants(xc, yc, x, y, color);
  }
}

void DUINO_SSD1306::fill_circle(int16_t xc, int16_t yc, int16_t r, SSD1306Color color)
{
  register int8_t x = 0;
  register int8_t y = r;
  register int8_t d = 3 - 2 * r;

  // same as draw_circle, but vlines instead of locus pixels
  while(y >= x)
  {
    fill_quadrants(xc, yc, x, y, color);
    x++;

    if(d > 0)
    {
      y--; 
      d = d + 4 * (x - y) + 10;
    }
    else
    {
      d = d + 4 * x + 6;
    }
    fill_quadrants(xc, yc, x, y, color);
  }
}

void DUINO_SSD1306::fill_rect(int16_t x, int16_t y, int16_t w, int16_t h, SSD1306Color color)
{
  // draw filled rectangle as a series of vlines
  for(int16_t i = x; i < x + w; ++i)
    draw_vline(i, y, h, color);
}

void DUINO_SSD1306::fill_screen(SSD1306Color color)
{
  // full-screen filled rectangle
  fill_rect(0, 0, SSD1306_LCDWIDTH, SSD1306_LCDHEIGHT, color);
}

void DUINO_SSD1306::draw_char(int16_t x, int16_t y, unsigned char c, SSD1306Color color)
{
  // bound check
  if(((x + 5) < 0) || (x >= SSD1306_LCDWIDTH) || ((y + 7) < 0) || (y >= SSD1306_LCDHEIGHT))
    return;

  // draw pixels from ASCII 5x7 font map
  for(int8_t i = 0; i < 5; ++i)
  {
    uint8_t line = pgm_read_byte(&font5x7[c * 5 + i]);
    for(int8_t j = 0; j < 8; ++j, line >>= 1)
    {
      if(line & 1)
      {
        draw_pixel(x + i, y + j, color);
      }
    }
  }
}

void DUINO_SSD1306::draw_text(int16_t x, int16_t y, const char * text, SSD1306Color color)
{
  // draw characters with 1-pixel spacing
  register int8_t i = 0;
  while(*(text + i))
    draw_char(x + 6 * i++, y, *(text + i), color);
}

void DUINO_SSD1306::draw_icon_16(int16_t x, int16_t y, const unsigned char * map, unsigned char c, SSD1306Color color)
{
  // bound check
  if(((x + 16) < 0) || (x >= SSD1306_LCDWIDTH) || ((y + 16) < 0) || (y >= SSD1306_LCDHEIGHT))
    return;

  // draw pixels from icon map
  for(int8_t i = 0; i < 16; ++i)
  {
    uint8_t line1 = pgm_read_byte(&map[c * 32 + i * 2]);
    uint8_t line2 = pgm_read_byte(&map[c * 32 + i * 2 + 1]);
    for(int8_t j = 0; j < 8; ++j, line1 >>= 1, line2 >>= 1)
    {
      if(line1 & 1)
      {
        draw_pixel(x + i, y + j, color);
      }
      if(line2 & 1)
      {
        draw_pixel(x + i, y + j + 8, color);
      }
    }
  }
}

void DUINO_SSD1306::draw_du_logo_lg(int16_t x, int16_t y, SSD1306Color color)
{
  fill_circle(x + 19, y + 19, 19, color);
  fill_circle(x + 59, y + 19, 19, color);
  fill_rect(x, y, 19, 21, color);
  fill_rect(x + 39, y, 40, 21, color);
}

void DUINO_SSD1306::draw_du_logo_sm(int16_t x, int16_t y, SSD1306Color color)
{
  draw_vline(x, y, 5, color);
  draw_vline(x + 1, y, 6, color);
  fill_rect(x + 2, y, 3, 7, color);
  draw_vline(x + 5, y + 1, 5, color);
  draw_vline(x + 6, y + 2, 3, color);
  draw_vline(x + 7, y, 5, color);
  draw_vline(x + 8, y, 6, color);
  fill_rect(x + 9, y, 3, 7, color);
  draw_vline(x + 12, y, 6, color);
  draw_vline(x + 13, y, 5, color);
}

void DUINO_SSD1306::draw_logick_logo(int16_t x, int16_t y, SSD1306Color color)
{
  fill_rect(x, y, 2, 10, color);
  fill_rect(x + 3, y + 4, 2, 6, color);
  draw_hline(x + 5, y + 4, 2, color);
  draw_hline(x + 5, y + 9, 2, color);
  fill_rect(x + 7, y + 4, 2, 6, color);
  fill_rect(x + 10, y + 4, 2, 6, color);
  draw_hline(x + 12, y + 4, 2, color);
  draw_hline(x + 12, y + 9, 2, color);
  fill_rect(x + 14, y + 4, 2, 8, color);
  draw_hline(x, y + 11, 14, color);
  fill_rect(x + 17, y + 4, 2, 6, color);
  fill_rect(x + 20, y + 4, 2, 6, color);
  draw_hline(x + 22, y + 4, 3, color);
  draw_hline(x + 22, y + 9, 3, color);
  fill_rect(x + 26, y, 2, 10, color);
  draw_vline(x + 28, y + 5, 2, color);
  draw_vline(x + 29, y + 4, 4, color);
  draw_vline(x + 30, y + 3, 2, color);
  draw_vline(x + 30, y + 7, 2, color);
  draw_vline(x + 31, y + 2, 2, color);
  draw_vline(x + 31, y + 8, 2, color);
  draw_vline(x + 32, y + 9, 2, color);
  draw_vline(x + 33, y + 10, 2, color);
  draw_vline(x + 34, y + 11, 2, color);
  draw_vline(x + 35, y + 12, 2, color);
  draw_vline(x + 36, y + 13, 2, color);
  draw_vline(x + 37, y + 14, 2, color);
}

inline void DUINO_SSD1306::draw_quadrants(int16_t xc, int16_t yc, int16_t x, int16_t y, SSD1306Color color)
{
  draw_pixel(xc + x, yc + y, color);
  draw_pixel(xc + x, yc - y, color);
  draw_pixel(xc + y, yc + x, color);
  draw_pixel(xc + y, yc - x, color);
  draw_pixel(xc - x, yc + y, color);
  draw_pixel(xc - x, yc - y, color);
  draw_pixel(xc - y, yc + x, color);
  draw_pixel(xc - y, yc - x, color);
}

inline void DUINO_SSD1306::fill_quadrants(int16_t xc, int16_t yc, int16_t x, int16_t y, SSD1306Color color)
{
  draw_vline(xc + x, yc - y, y + y, color);
  draw_vline(xc + y, yc - x, x + x, color);
  draw_vline(xc - x, yc - y, y + y, color);
  draw_vline(xc - y, yc - x, x + x, color);
}
