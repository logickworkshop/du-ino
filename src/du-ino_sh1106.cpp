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

#include <stdlib.h>
#include <SPI.h>
#include <avr/pgmspace.h>
#include "du-ino_font5x7.h"
#include "du-ino_SH1106.h"

static uint8_t buffer[SH1106_LCDHEIGHT * SH1106_LCDWIDTH / 8];

DUINO_SH1106::DUINO_SH1106(uint8_t ss, uint8_t dc)
  : pin_ss_(ss)
  , pin_dc_(dc)
{
  pinMode(pin_ss_, OUTPUT);
  pinMode(pin_dc_, OUTPUT);
}

void DUINO_SH1106::sh1106_command(uint8_t command)
{
  digitalWrite(pin_ss_, HIGH);
  digitalWrite(pin_dc_, LOW);
  digitalWrite(pin_ss_, LOW);
  (void)SPI.transfer(command);
  digitalWrite(pin_ss_, HIGH);
}

void DUINO_SH1106::begin()
{
  // hold chip deselect
  digitalWrite(pin_ss_, HIGH);

  // hold DC low
  digitalWrite(pin_dc_, LOW);

  // initialize SPI
  SPI.begin();
  SPI.setBitOrder(MSBFIRST);
  SPI.setDataMode(SPI_MODE0);
  SPI.setClockDivider(SPI_CLOCK_DIV2);

  // initialization sequence
  sh1106_command(SH1106_DISPLAYOFF);
  sh1106_command(SH1106_SETDISPLAYCLOCKDIV);
  sh1106_command(0x80);
  sh1106_command(SH1106_SETMULTIPLEX);
  sh1106_command(SH1106_LCDHEIGHT - 1);
  sh1106_command(SH1106_SETDISPLAYOFFSET);
  sh1106_command(0x00);
  sh1106_command(SH1106_SETSTARTLINE);
  sh1106_command(SH1106_SEGREMAP | 0x01);
  sh1106_command(SH1106_COMSCANDEC);
  sh1106_command(SH1106_SETCOMPINS);
  sh1106_command(0x12);
  sh1106_command(SH1106_SETCONTRAST);
  sh1106_command(0xCF);
  sh1106_command(SH1106_SETPRECHARGE);
  sh1106_command(0xF1);
  sh1106_command(SH1106_SETVCOMDETECT);
  sh1106_command(0x40);
  sh1106_command(SH1106_DISPLAYALLON_RESUME);
  sh1106_command(SH1106_NORMALDISPLAY);
  sh1106_command(SH1106_DISPLAYON);
}

void DUINO_SH1106::display(uint8_t col_start, uint8_t col_end, uint8_t page_start, uint8_t page_end)
{
  uint8_t page, col, b;
  
  // TODO: use the column and page values, or rework with new (or no) parameters and update examples
  for (page = page_start; page <= page_end; ++page)
  {
    // FIXME: command needs a #define
    sh1106_command(SH1106_SETPAGEADDR | page);
    sh1106_command(SH1106_SETLOWCOLUMN | (col_start + 2) & 0x0F);
    sh1106_command(SH1106_SETHIGHCOLUMN | (col_start + 2) >> 4);
    
    digitalWrite(pin_ss_, HIGH);
    digitalWrite(pin_dc_, HIGH);
    digitalWrite(pin_ss_, LOW);

    b = 1;
    for (col = col_start; col <= col_end; ++col, ++b)
    {
      (void)SPI.transfer(buffer[page * SH1106_LCDWIDTH + col]);
      if (b % 16)
      {
        digitalWrite(pin_ss_, HIGH);
        digitalWrite(pin_ss_, LOW);
      }
    }

    digitalWrite(pin_ss_, HIGH);
  }
}

void DUINO_SH1106::display()
{
  display(0, 127, 0, 7);
}

void DUINO_SH1106::clear_display()
{
  // clear buffer
  memset(buffer, 0, SH1106_LCDHEIGHT * SH1106_LCDWIDTH / 8);
}

void DUINO_SH1106::draw_pixel(int16_t x, int16_t y, Color color)
{
  // bound check
  if ((x < 0) || (x >= SH1106_LCDWIDTH) || (y < 0) || (y >= SH1106_LCDHEIGHT))
  {
    return;
  }

  // set pixel value
  switch (color)
  {
    case Black:
      buffer[x + (y / 8) * SH1106_LCDWIDTH] &= ~(1 << (y & 7));
      break;
    case White:
      buffer[x + (y / 8) * SH1106_LCDWIDTH] |=  (1 << (y & 7));
      break;
    case Inverse:
      buffer[x + (y / 8) * SH1106_LCDWIDTH] ^=  (1 << (y & 7));
      break;
  }
}

void DUINO_SH1106::draw_hline(int16_t x, int16_t y, int16_t w, Color color)
{
  // bound check
  if ((y < 0) || (y >= SH1106_LCDHEIGHT))
  {
    return;
  }

  // clip at edges of display
  if (x < 0)
  {
    w += x;
    x = 0;
  }
  if ((x + w) > SH1106_LCDWIDTH)
  {
    w = (SH1106_LCDWIDTH - x);
  }

  // check width
  if (w <= 0)
  {
    return;
  }

  // initialize buffer pointer
  register uint8_t * bp = buffer + ((y / 8) * SH1106_LCDWIDTH) + x;

  // initialize pixel mask
  register uint8_t mask = 1 << (y & 7);

  switch (color)
  {
    case Black:
      mask = ~mask;
      while (w--)
      {
        *bp++ &= mask;
      }
      break;
    case White:
      while (w--)
      {
        *bp++ |= mask;
      }
      break;
    case Inverse:
      while (w--)
      {
        *bp++ ^= mask;
      }
      break;
  }
}

void DUINO_SH1106::draw_vline(int16_t x, int16_t y, int16_t h, Color color)
{
  // bound check
  if ((x < 0) || (x >= SH1106_LCDWIDTH))
  {
    return;
  }

  // clip at edges of display
  if (y < 0)
  {
    h += y;
    y = 0;
  }
  if ((y + h) > SH1106_LCDHEIGHT)
  {
    h = (SH1106_LCDHEIGHT - y);
  }

  // check height
  if (h <= 0)
  {
    return;
  }

  // use local byte registers for coordinates
  register uint8_t ry = y;
  register uint8_t rh = h;

  // set up the pointer for fast movement through the buffer
  register uint8_t *bp = buffer + ((ry / 8) * SH1106_LCDWIDTH) + x;

  // first partial byte
  register uint8_t mod = (ry & 7);
  if (mod)
  {
    // mask high bits
    mod = 8 - mod;
    static uint8_t premask[8] = {0x00, 0x80, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC, 0xFE};
    register uint8_t mask = premask[mod];

    // adjust the mask
    if (rh < mod)
    {
      mask &= (0XFF >> (mod - rh));
    }

    switch (color)
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
    if (rh < mod)
    {
      return;
    }

    rh -= mod;
    bp += SH1106_LCDWIDTH;
  }

  // solid bytes
  if (rh >= 8)
  {
    if (color == Inverse)
    {
      do
      {
        *bp = ~(*bp);
        bp += SH1106_LCDWIDTH;
        rh -= 8;
      }
      while (rh >= 8);
    }
    else
    {
      register uint8_t val = (color == White) ? 0xFF : 0x00;

      do
      {
        *bp = val;
        bp += SH1106_LCDWIDTH;
        rh -= 8;
      }
      while (rh >= 8);
    }
  }

  // last partial byte
  if (rh)
  {
    // mask low bits
    mod = rh & 7;
    static uint8_t postmask[8] = {0x00, 0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x7F};
    register uint8_t mask = postmask[mod];

    switch (color)
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

void DUINO_SH1106::draw_circle(int16_t xc, int16_t yc, int16_t r, Color color)
{
  register int8_t x = 0;
  register int8_t y = r;
  register int8_t d = 3 - 2 * r;

  // Bresenham raster circle algorithm
  while (y >= x)
  {
    draw_quadrants(xc, yc, x, y, color);
    x++;

    if (d > 0)
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

void DUINO_SH1106::fill_circle(int16_t xc, int16_t yc, int16_t r, Color color)
{
  register int8_t x = 0;
  register int8_t y = r;
  register int8_t d = 3 - 2 * r;

  // same as draw_circle, but vlines instead of locus pixels
  while (y >= x)
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

void DUINO_SH1106::fill_rect(int16_t x, int16_t y, int16_t w, int16_t h, Color color)
{
  // draw filled rectangle as a series of vlines
  for (int16_t i = x; i < x + w; ++i)
  {
    draw_vline(i, y, h, color);
  }
}

void DUINO_SH1106::fill_screen(Color color)
{
  // full-screen filled rectangle
  fill_rect(0, 0, SH1106_LCDWIDTH, SH1106_LCDHEIGHT, color);
}

void DUINO_SH1106::draw_char(int16_t x, int16_t y, unsigned char c, Color color)
{
  // bound check
  if (((x + 5) < 0) || (x >= SH1106_LCDWIDTH) || ((y + 7) < 0) || (y >= SH1106_LCDHEIGHT))
  {
    return;
  }

  // draw pixels from ASCII 5x7 font map
  for (int8_t i = 0; i < 5; ++i)
  {
    uint8_t line = pgm_read_byte(&font5x7[c * 5 + i]);
    for (int8_t j = 0; j < 8; ++j, line >>= 1)
    {
      if (line & 1)
      {
        draw_pixel(x + i, y + j, color);
      }
    }
  }
}

void DUINO_SH1106::draw_text(int16_t x, int16_t y, const char * text, Color color)
{
  // draw characters with 1-pixel spacing
  register int8_t i = 0;
  while (*(text + i))
  {
    draw_char(x + 6 * i++, y, *(text + i), color);
  }
}

void DUINO_SH1106::draw_bitmap_7(int16_t x, int16_t y, const unsigned char * map, unsigned char c, Color color)
{
  // bound check
  if (((x + 7) < 0) || (x >= SH1106_LCDWIDTH) || ((y + 7) < 0) || (y >= SH1106_LCDHEIGHT))
  {
    return;
  }

  // draw pixels from map
  for (int8_t i = 0; i < 7; ++i)
  {
    uint8_t line = pgm_read_byte(&map[c * 7 + i]);
    for (int8_t j = 0; j < 7; ++j, line >>= 1)
    {
      if (line & 1)
      {
        draw_pixel(x + i, y + j, color);
      }
    }
  }
}

void DUINO_SH1106::draw_bitmap_8(int16_t x, int16_t y, const unsigned char * map, unsigned char c, Color color)
{
  // bound check
  if (((x + 8) < 0) || (x >= SH1106_LCDWIDTH) || ((y + 8) < 0) || (y >= SH1106_LCDHEIGHT))
  {
    return;
  }

  // draw pixels from map
  for (int8_t i = 0; i < 8; ++i)
  {
    uint8_t line = pgm_read_byte(&map[c * 8 + i]);
    for (int8_t j = 0; j < 8; ++j, line >>= 1)
    {
      if (line & 1)
      {
        draw_pixel(x + i, y + j, color);
      }
    }
  }
}

void DUINO_SH1106::draw_du_logo_lg(int16_t x, int16_t y, Color color)
{
  fill_circle(x + 19, y + 19, 19, color);
  fill_circle(x + 59, y + 19, 19, color);
  fill_rect(x, y, 19, 21, color);
  fill_rect(x + 39, y, 40, 21, color);
}

void DUINO_SH1106::draw_du_logo_sm(int16_t x, int16_t y, Color color)
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

void DUINO_SH1106::draw_logick_logo(int16_t x, int16_t y, Color color)
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

inline void DUINO_SH1106::draw_quadrants(int16_t xc, int16_t yc, int16_t x, int16_t y, Color color)
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

inline void DUINO_SH1106::fill_quadrants(int16_t xc, int16_t yc, int16_t x, int16_t y, Color color)
{
  draw_vline(xc + x, yc - y, y + y, color);
  draw_vline(xc + y, yc - x, x + x, color);
  draw_vline(xc - x, yc - y, y + y, color);
  draw_vline(xc - y, yc - x, x + x, color);
}

DUINO_SH1106 Display(5, 4);
