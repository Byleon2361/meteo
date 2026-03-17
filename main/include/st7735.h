#pragma once

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include <stdint.h>

#define ST7735_PIN_MOSI 13
#define ST7735_PIN_SCLK 14
#define ST7735_PIN_CS 26
#define ST7735_PIN_DC 27
#define ST7735_PIN_RST 25
#define ST7735_PIN_BL -1

#define ST7735_SPI_HOST SPI2_HOST
#define ST7735_SPI_FREQ (26 * 1000 * 1000) // 26 МГц

#define ST7735_WIDTH 128
#define ST7735_HEIGHT 160

#define ST7735_X_OFFSET 0
#define ST7735_Y_OFFSET 0

#define ST7735_BLACK 0x0000
#define ST7735_WHITE 0xFFFF
#define ST7735_RED 0xF800
#define ST7735_GREEN 0x07E0
#define ST7735_BLUE 0x001F
#define ST7735_YELLOW 0xFFE0
#define ST7735_CYAN 0x07FF
#define ST7735_MAGENTA 0xF81F
#define ST7735_ORANGE 0xFD20
#define ST7735_GRAY 0x8410
#define ST7735_DARKGRAY 0x4208

void st7735_init(void);

void st7735_fill_screen(uint16_t color);

void st7735_fill_rect(
        int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);

void st7735_draw_hline(int16_t x, int16_t y, int16_t len, uint16_t color);

void st7735_draw_char(
        int16_t x, int16_t y, char c, uint16_t fg, uint16_t bg, uint8_t scale);

void st7735_draw_string(
        int16_t x,
        int16_t y,
        const char* str,
        uint16_t fg,
        uint16_t bg,
        uint8_t scale);

void st7735_draw_int(
        int16_t x,
        int16_t y,
        int32_t val,
        uint16_t fg,
        uint16_t bg,
        uint8_t scale);

void st7735_draw_float(
        int16_t x,
        int16_t y,
        float val,
        uint8_t decimals,
        uint16_t fg,
        uint16_t bg,
        uint8_t scale);