#include "st7735.h"

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>
#include <string.h>

static const char* TAG = "ST7735";

#define ST7735_NOP 0x00
#define ST7735_SWRESET 0x01
#define ST7735_SLPOUT 0x11
#define ST7735_NORON 0x13
#define ST7735_INVOFF 0x20
#define ST7735_INVON 0x21
#define ST7735_DISPOFF 0x28
#define ST7735_DISPON 0x29
#define ST7735_CASET 0x2A
#define ST7735_RASET 0x2B
#define ST7735_RAMWR 0x2C
#define ST7735_MADCTL 0x36
#define ST7735_COLMOD 0x3A
#define ST7735_FRMCTR1 0xB1
#define ST7735_FRMCTR2 0xB2
#define ST7735_FRMCTR3 0xB3
#define ST7735_INVCTR 0xB4
#define ST7735_PWCTR1 0xC0
#define ST7735_PWCTR2 0xC1
#define ST7735_PWCTR3 0xC2
#define ST7735_PWCTR4 0xC3
#define ST7735_PWCTR5 0xC4
#define ST7735_VMCTR1 0xC5
#define ST7735_GMCTRP1 0xE0
#define ST7735_GMCTRN1 0xE1

#define MADCTL_MY 0x80
#define MADCTL_MX 0x40
#define MADCTL_MV 0x20
#define MADCTL_RGB 0x00
#define MADCTL_BGR 0x08

static spi_device_handle_t s_spi = NULL;

static void dc_set(uint8_t val)
{
    gpio_set_level(ST7735_PIN_DC, val);
}

static void send_cmd(uint8_t cmd)
{
    dc_set(0);
    spi_transaction_t t = {
            .length = 8,
            .tx_buffer = &cmd,
            .flags = 0,
    };
    spi_device_polling_transmit(s_spi, &t);
}

static void send_data(const uint8_t* data, size_t len)
{
    if (len == 0)
        return;
    dc_set(1);
    spi_transaction_t t = {
            .length = len * 8,
            .tx_buffer = data,
            .flags = 0,
    };
    spi_device_polling_transmit(s_spi, &t);
}

static void send_byte(uint8_t b)
{
    send_data(&b, 1);
}

static void send_word(uint16_t w)
{
    uint8_t buf[2] = {w >> 8, w & 0xFF};
    send_data(buf, 2);
}

static const uint8_t font5x7[][5] = {
        {0x00, 0x00, 0x00, 0x00, 0x00}, // 32 ' '
        {0x00, 0x00, 0x5F, 0x00, 0x00}, // 33 '!'
        {0x00, 0x07, 0x00, 0x07, 0x00}, // 34 '"'
        {0x14, 0x7F, 0x14, 0x7F, 0x14}, // 35 '#'
        {0x24, 0x2A, 0x7F, 0x2A, 0x12}, // 36 '$'
        {0x23, 0x13, 0x08, 0x64, 0x62}, // 37 '%'
        {0x36, 0x49, 0x55, 0x22, 0x50}, // 38 '&'
        {0x00, 0x05, 0x03, 0x00, 0x00}, // 39 '\''
        {0x00, 0x1C, 0x22, 0x41, 0x00}, // 40 '('
        {0x00, 0x41, 0x22, 0x1C, 0x00}, // 41 ')'
        {0x14, 0x08, 0x3E, 0x08, 0x14}, // 42 '*'
        {0x08, 0x08, 0x3E, 0x08, 0x08}, // 43 '+'
        {0x00, 0x50, 0x30, 0x00, 0x00}, // 44 ','
        {0x08, 0x08, 0x08, 0x08, 0x08}, // 45 '-'
        {0x00, 0x60, 0x60, 0x00, 0x00}, // 46 '.'
        {0x20, 0x10, 0x08, 0x04, 0x02}, // 47 '/'
        {0x3E, 0x51, 0x49, 0x45, 0x3E}, // 48 '0'
        {0x00, 0x42, 0x7F, 0x40, 0x00}, // 49 '1'
        {0x42, 0x61, 0x51, 0x49, 0x46}, // 50 '2'
        {0x21, 0x41, 0x45, 0x4B, 0x31}, // 51 '3'
        {0x18, 0x14, 0x12, 0x7F, 0x10}, // 52 '4'
        {0x27, 0x45, 0x45, 0x45, 0x39}, // 53 '5'
        {0x3C, 0x4A, 0x49, 0x49, 0x30}, // 54 '6'
        {0x01, 0x71, 0x09, 0x05, 0x03}, // 55 '7'
        {0x36, 0x49, 0x49, 0x49, 0x36}, // 56 '8'
        {0x06, 0x49, 0x49, 0x29, 0x1E}, // 57 '9'
        {0x00, 0x36, 0x36, 0x00, 0x00}, // 58 ':'
        {0x00, 0x56, 0x36, 0x00, 0x00}, // 59 ';'
        {0x08, 0x14, 0x22, 0x41, 0x00}, // 60 '<'
        {0x14, 0x14, 0x14, 0x14, 0x14}, // 61 '='
        {0x00, 0x41, 0x22, 0x14, 0x08}, // 62 '>'
        {0x02, 0x01, 0x51, 0x09, 0x06}, // 63 '?'
        {0x32, 0x49, 0x79, 0x41, 0x3E}, // 64 '@'
        {0x7E, 0x11, 0x11, 0x11, 0x7E}, // 65 'A'
        {0x7F, 0x49, 0x49, 0x49, 0x36}, // 66 'B'
        {0x3E, 0x41, 0x41, 0x41, 0x22}, // 67 'C'
        {0x7F, 0x41, 0x41, 0x22, 0x1C}, // 68 'D'
        {0x7F, 0x49, 0x49, 0x49, 0x41}, // 69 'E'
        {0x7F, 0x09, 0x09, 0x09, 0x01}, // 70 'F'
        {0x3E, 0x41, 0x49, 0x49, 0x7A}, // 71 'G'
        {0x7F, 0x08, 0x08, 0x08, 0x7F}, // 72 'H'
        {0x00, 0x41, 0x7F, 0x41, 0x00}, // 73 'I'
        {0x20, 0x40, 0x41, 0x3F, 0x01}, // 74 'J'
        {0x7F, 0x08, 0x14, 0x22, 0x41}, // 75 'K'
        {0x7F, 0x40, 0x40, 0x40, 0x40}, // 76 'L'
        {0x7F, 0x02, 0x04, 0x02, 0x7F}, // 77 'M'
        {0x7F, 0x04, 0x08, 0x10, 0x7F}, // 78 'N'
        {0x3E, 0x41, 0x41, 0x41, 0x3E}, // 79 'O'
        {0x7F, 0x09, 0x09, 0x09, 0x06}, // 80 'P'
        {0x3E, 0x41, 0x51, 0x21, 0x5E}, // 81 'Q'
        {0x7F, 0x09, 0x19, 0x29, 0x46}, // 82 'R'
        {0x46, 0x49, 0x49, 0x49, 0x31}, // 83 'S'
        {0x01, 0x01, 0x7F, 0x01, 0x01}, // 84 'T'
        {0x3F, 0x40, 0x40, 0x40, 0x3F}, // 85 'U'
        {0x1F, 0x20, 0x40, 0x20, 0x1F}, // 86 'V'
        {0x3F, 0x40, 0x38, 0x40, 0x3F}, // 87 'W'
        {0x63, 0x14, 0x08, 0x14, 0x63}, // 88 'X'
        {0x07, 0x08, 0x70, 0x08, 0x07}, // 89 'Y'
        {0x61, 0x51, 0x49, 0x45, 0x43}, // 90 'Z'
        {0x00, 0x7F, 0x41, 0x41, 0x00}, // 91 '['
        {0x02, 0x04, 0x08, 0x10, 0x20}, // 92 '\'
        {0x00, 0x41, 0x41, 0x7F, 0x00}, // 93 ']'
        {0x04, 0x02, 0x01, 0x02, 0x04}, // 94 '^'
        {0x40, 0x40, 0x40, 0x40, 0x40}, // 95 '_'
        {0x00, 0x01, 0x02, 0x04, 0x00}, // 96 '`'
        {0x20, 0x54, 0x54, 0x54, 0x78}, // 97 'a'
        {0x7F, 0x48, 0x44, 0x44, 0x38}, // 98 'b'
        {0x38, 0x44, 0x44, 0x44, 0x20}, // 99 'c'
        {0x38, 0x44, 0x44, 0x48, 0x7F}, // 100 'd'
        {0x38, 0x54, 0x54, 0x54, 0x18}, // 101 'e'
        {0x08, 0x7E, 0x09, 0x01, 0x02}, // 102 'f'
        {0x0C, 0x52, 0x52, 0x52, 0x3E}, // 103 'g'
        {0x7F, 0x08, 0x04, 0x04, 0x78}, // 104 'h'
        {0x00, 0x44, 0x7D, 0x40, 0x00}, // 105 'i'
        {0x20, 0x40, 0x44, 0x3D, 0x00}, // 106 'j'
        {0x7F, 0x10, 0x28, 0x44, 0x00}, // 107 'k'
        {0x00, 0x41, 0x7F, 0x40, 0x00}, // 108 'l'
        {0x7C, 0x04, 0x18, 0x04, 0x78}, // 109 'm'
        {0x7C, 0x08, 0x04, 0x04, 0x78}, // 110 'n'
        {0x38, 0x44, 0x44, 0x44, 0x38}, // 111 'o'
        {0x7C, 0x14, 0x14, 0x14, 0x08}, // 112 'p'
        {0x08, 0x14, 0x14, 0x18, 0x7C}, // 113 'q'
        {0x7C, 0x08, 0x04, 0x04, 0x08}, // 114 'r'
        {0x48, 0x54, 0x54, 0x54, 0x20}, // 115 's'
        {0x04, 0x3F, 0x44, 0x40, 0x20}, // 116 't'
        {0x3C, 0x40, 0x40, 0x20, 0x7C}, // 117 'u'
        {0x1C, 0x20, 0x40, 0x20, 0x1C}, // 118 'v'
        {0x3C, 0x40, 0x30, 0x40, 0x3C}, // 119 'w'
        {0x44, 0x28, 0x10, 0x28, 0x44}, // 120 'x'
        {0x0C, 0x50, 0x50, 0x50, 0x3C}, // 121 'y'
        {0x44, 0x64, 0x54, 0x4C, 0x44}, // 122 'z'
        {0x00, 0x08, 0x36, 0x41, 0x00}, // 123 '{'
        {0x00, 0x00, 0x7F, 0x00, 0x00}, // 124 '|'
        {0x00, 0x41, 0x36, 0x08, 0x00}, // 125 '}'
        {0x10, 0x08, 0x08, 0x10, 0x08}, // 126 '~'
};

static void set_addr_window(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1)
{
    send_cmd(ST7735_CASET);
    send_word(x0 + ST7735_X_OFFSET);
    send_word(x1 + ST7735_X_OFFSET);

    send_cmd(ST7735_RASET);
    send_word(y0 + ST7735_Y_OFFSET);
    send_word(y1 + ST7735_Y_OFFSET);

    send_cmd(ST7735_RAMWR);
}

void st7735_init(void)
{
    spi_bus_config_t bus_cfg = {
            .mosi_io_num = ST7735_PIN_MOSI,
            .miso_io_num = -1,
            .sclk_io_num = ST7735_PIN_SCLK,
            .quadwp_io_num = -1,
            .quadhd_io_num = -1,
            .max_transfer_sz = ST7735_WIDTH * ST7735_HEIGHT * 2 + 8,
    };
    ESP_ERROR_CHECK(
            spi_bus_initialize(ST7735_SPI_HOST, &bus_cfg, SPI_DMA_CH_AUTO));

    spi_device_interface_config_t dev_cfg = {
            .clock_speed_hz = ST7735_SPI_FREQ,
            .mode = 0,
            .spics_io_num = ST7735_PIN_CS,
            .queue_size = 7,
    };
    ESP_ERROR_CHECK(spi_bus_add_device(ST7735_SPI_HOST, &dev_cfg, &s_spi));

    gpio_config_t io_conf = {
            .pin_bit_mask = (1ULL << ST7735_PIN_DC),
            .mode = GPIO_MODE_OUTPUT,
    };
    gpio_config(&io_conf);

    if (ST7735_PIN_RST >= 0) {
        io_conf.pin_bit_mask = (1ULL << ST7735_PIN_RST);
        gpio_config(&io_conf);
        gpio_set_level(ST7735_PIN_RST, 0);
        vTaskDelay(pdMS_TO_TICKS(10));
        gpio_set_level(ST7735_PIN_RST, 1);
        vTaskDelay(pdMS_TO_TICKS(120));
    }

    if (ST7735_PIN_BL >= 0) {
        io_conf.pin_bit_mask = (1ULL << ST7735_PIN_BL);
        gpio_config(&io_conf);
        gpio_set_level(ST7735_PIN_BL, 1);
    }

    send_cmd(ST7735_SWRESET);
    vTaskDelay(pdMS_TO_TICKS(150));
    send_cmd(ST7735_SLPOUT);
    vTaskDelay(pdMS_TO_TICKS(500));

    send_cmd(ST7735_FRMCTR1);
    send_byte(0x01);
    send_byte(0x2C);
    send_byte(0x2D);

    send_cmd(ST7735_FRMCTR2);
    send_byte(0x01);
    send_byte(0x2C);
    send_byte(0x2D);

    send_cmd(ST7735_FRMCTR3);
    send_byte(0x01);
    send_byte(0x2C);
    send_byte(0x2D);
    send_byte(0x01);
    send_byte(0x2C);
    send_byte(0x2D);

    send_cmd(ST7735_INVCTR);
    send_byte(0x07);

    send_cmd(ST7735_PWCTR1);
    send_byte(0xA2);
    send_byte(0x02);
    send_byte(0x84);
    send_cmd(ST7735_PWCTR2);
    send_byte(0xC5);
    send_cmd(ST7735_PWCTR3);
    send_byte(0x0A);
    send_byte(0x00);
    send_cmd(ST7735_PWCTR4);
    send_byte(0x8A);
    send_byte(0x2A);
    send_cmd(ST7735_PWCTR5);
    send_byte(0x8A);
    send_byte(0xEE);
    send_cmd(ST7735_VMCTR1);
    send_byte(0x0E);

    send_cmd(ST7735_INVOFF);

   send_cmd(ST7735_MADCTL);
   send_byte(MADCTL_MY | MADCTL_MX | MADCTL_RGB);
    

    send_cmd(ST7735_COLMOD);
    send_byte(0x05);

    send_cmd(ST7735_GMCTRP1);
    {
        uint8_t g[]
                = {0x02,
                   0x1c,
                   0x07,
                   0x12,
                   0x37,
                   0x32,
                   0x29,
                   0x2d,
                   0x29,
                   0x25,
                   0x2B,
                   0x39,
                   0x00,
                   0x01,
                   0x03,
                   0x10};
        send_data(g, sizeof(g));
    }
    send_cmd(ST7735_GMCTRN1);
    {
        uint8_t g[]
                = {0x03,
                   0x1d,
                   0x07,
                   0x06,
                   0x2E,
                   0x2C,
                   0x29,
                   0x2D,
                   0x2E,
                   0x2E,
                   0x37,
                   0x3F,
                   0x00,
                   0x00,
                   0x02,
                   0x10};
        send_data(g, sizeof(g));
    }

    send_cmd(ST7735_NORON);
    vTaskDelay(pdMS_TO_TICKS(10));
    send_cmd(ST7735_DISPON);
    vTaskDelay(pdMS_TO_TICKS(100));

    ESP_LOGI(
            TAG, "ST7735 инициализирован (%dx%d)", ST7735_WIDTH, ST7735_HEIGHT);
}

void st7735_fill_screen(uint16_t color)
{
    st7735_fill_rect(0, 0, ST7735_WIDTH, ST7735_HEIGHT, color);
}

void st7735_fill_rect(
        int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
    if (x >= ST7735_WIDTH || y >= ST7735_HEIGHT)
        return;
    if (x + w > ST7735_WIDTH)
        w = ST7735_WIDTH - x;
    if (y + h > ST7735_HEIGHT)
        h = ST7735_HEIGHT - y;

    set_addr_window(x, y, x + w - 1, y + h - 1);

    uint8_t hi = color >> 8, lo = color & 0xFF;

    static uint8_t line_buf[ST7735_WIDTH * 2];
    for (int i = 0; i < w; i++) {
        line_buf[i * 2] = hi;
        line_buf[i * 2 + 1] = lo;
    }

    dc_set(1);
    for (int row = 0; row < h; row++) {
        spi_transaction_t t = {
                .length = w * 16,
                .tx_buffer = line_buf,
        };
        spi_device_polling_transmit(s_spi, &t);
    }
}

void st7735_draw_hline(int16_t x, int16_t y, int16_t len, uint16_t color)
{
    st7735_fill_rect(x, y, len, 1, color);
}

void st7735_draw_char(
        int16_t x, int16_t y, char c, uint16_t fg, uint16_t bg, uint8_t scale)
{
    if (c < 32 || c > 126)
        c = '?';
    const uint8_t* glyph = font5x7[c - 32];

    for (int col = 0; col < 5; col++) {
        uint8_t line = glyph[col];
        for (int row = 0; row < 7; row++) {
            uint16_t color = (line & (1 << row)) ? fg : bg;
            st7735_fill_rect(
                    x + col * scale, y + row * scale, scale, scale, color);
        }
    }
    st7735_fill_rect(x + 5 * scale, y, scale, 7 * scale, bg);
}

void st7735_draw_string(
        int16_t x,
        int16_t y,
        const char* str,
        uint16_t fg,
        uint16_t bg,
        uint8_t scale)
{
    int16_t cx = x;
    while (*str) {
        st7735_draw_char(cx, y, *str, fg, bg, scale);
        cx += 6 * scale;
        str++;
    }
}

void st7735_draw_int(
        int16_t x,
        int16_t y,
        int32_t val,
        uint16_t fg,
        uint16_t bg,
        uint8_t scale)
{
    char buf[16];
    snprintf(buf, sizeof(buf), "%ld", (long)val);
    st7735_draw_string(x, y, buf, fg, bg, scale);
}

void st7735_draw_float(
        int16_t x,
        int16_t y,
        float val,
        uint8_t decimals,
        uint16_t fg,
        uint16_t bg,
        uint8_t scale)
{
    char fmt[8], buf[16];
    snprintf(fmt, sizeof(fmt), "%%.%df", decimals);
    snprintf(buf, sizeof(buf), fmt, val);
    st7735_draw_string(x, y, buf, fg, bg, scale);
}