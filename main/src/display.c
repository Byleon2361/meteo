#include "display.h"
#include "dht22.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "mq135.h"
#include "pms5003.h"
#include "sensor_data.h"
#include "st7735.h"
#include <stdio.h>
#include <string.h>

static const char* TAG = "DISPLAY";

#define W ST7735_WIDTH
#define H ST7735_HEIGHT

#define MARGIN 3
#define HEADER_H 14
#define ROW_H 13
#define COL_W (W / 2)
#define LABEL_X MARGIN
#define VALUE_X 40

static void draw_header(void)
{
    st7735_fill_rect(0, 0, W, HEADER_H, ST7735_BLUE);
    st7735_draw_string(MARGIN, 3, "ESP32 WEATHER STATION", ST7735_WHITE, ST7735_BLUE, 1);
}

static void draw_cell(int16_t col, int16_t row, const char* label,
                      const char* value, uint16_t value_color)
{
    int16_t x = col * COL_W;
    int16_t y = HEADER_H + 2 + row * ROW_H;

    st7735_fill_rect(x, y, COL_W - 1, ROW_H - 1, ST7735_BLACK);
    st7735_draw_string(x + LABEL_X, y + 1, label, ST7735_GRAY, ST7735_BLACK, 1);
    st7735_draw_string(x + VALUE_X, y, value, value_color, ST7735_BLACK, 1);
}

static void update_values(void)
{
    char buf[16];

    float temperature_dht, humidity;
    uint8_t dht_valid;
    sensor_data_get_dht(&temperature_dht, &humidity, &dht_valid);

    float temperature_bmp, pressure;
    uint8_t bmp_valid;
    sensor_data_get_bmp(&temperature_bmp, &pressure, &bmp_valid);

    float temperature = sensor_data_get_temp_avg();

    float co2_ppm, lpg_ppm, co_ppm, nh3_ppm;
    sensor_data_get_mq(&co2_ppm, &lpg_ppm, &co_ppm, &nh3_ppm);

    uint16_t pm1_0, pm2_5, pm10;
    uint8_t pms_valid;
    sensor_data_get_pms5003(&pm1_0, &pm2_5, &pm10, &pms_valid);

    if (dht_valid || bmp_valid) {
        snprintf(buf, sizeof(buf), "%.1fC", temperature);
        draw_cell(0, 0, "TEMP", buf, ST7735_WHITE);
    } else {
        draw_cell(0, 0, "TEMP", "ERR", ST7735_RED);
    }

    if (dht_valid) {
        snprintf(buf, sizeof(buf), "%.0f%%", humidity);
        draw_cell(1, 0, "HUM", buf, ST7735_WHITE);
    } else {
        draw_cell(1, 0, "HUM", "ERR", ST7735_RED);
    }

    if (bmp_valid) {
        snprintf(buf, sizeof(buf), "%.0fmm", pressure);
        draw_cell(0, 1, "PRES", buf, ST7735_WHITE);
    } else {
        draw_cell(0, 1, "PRES", "ERR", ST7735_RED);
    }

    uint16_t co2_color = co2_ppm > 1000 ? ST7735_RED
            : co2_ppm > 700             ? ST7735_YELLOW
                                        : ST7735_WHITE;
    snprintf(buf, sizeof(buf), "%.0f", co2_ppm);
    draw_cell(1, 1, "CO2", buf, co2_color);

    uint16_t co_color = co_ppm > 50 ? ST7735_RED : ST7735_WHITE;
    snprintf(buf, sizeof(buf), "%.1f", co_ppm);
    draw_cell(0, 2, "CO", buf, co_color);

    snprintf(buf, sizeof(buf), "%.1f", nh3_ppm);
    draw_cell(1, 2, "NH3", buf, ST7735_WHITE);

    snprintf(buf, sizeof(buf), "%.1f", lpg_ppm);
    draw_cell(0, 3, "LPG", buf, ST7735_WHITE);

    if (pms_valid) {
        uint16_t pm_color = pm2_5 > 35 ? ST7735_RED
                : pm2_5 > 12           ? ST7735_YELLOW
                                       : ST7735_WHITE;
        snprintf(buf, sizeof(buf), "%u", pm2_5);
        draw_cell(1, 3, "PM2.5", buf, pm_color);
        snprintf(buf, sizeof(buf), "%u", pm1_0);
        draw_cell(0, 4, "PM1", buf, ST7735_WHITE);
        snprintf(buf, sizeof(buf), "%u", pm10);
        draw_cell(1, 4, "PM10", buf, ST7735_WHITE);
    } else {
        draw_cell(1, 3, "PM2.5", "..", ST7735_GRAY);
        draw_cell(0, 4, "PM1", "..", ST7735_GRAY);
        draw_cell(1, 4, "PM10", "..", ST7735_GRAY);
    }

    ESP_LOGI(TAG, "========== СЕНСОРНЫЕ ДАННЫЕ ==========");
    ESP_LOGI(TAG, "Температура (итог): %.1f C", temperature);
    if (dht_valid) {
        ESP_LOGI(TAG, "DHT22: %.1f C, Влажность: %.1f%%", temperature_dht, humidity);
    } else {
        ESP_LOGW(TAG, "DHT22: данные невалидны");
    }
    if (bmp_valid) {
        ESP_LOGI(TAG, "BMP280: %.1f C, Давление: %.1f мм рт.ст.", temperature_bmp, pressure);
    } else {
        ESP_LOGW(TAG, "BMP280: данные невалидны");
    }
    ESP_LOGI(TAG, "MQ-135: CO2 %.0f, CO %.1f, NH3 %.1f, LPG %.1f",
             co2_ppm, co_ppm, nh3_ppm, lpg_ppm);
    if (pms_valid) {
        ESP_LOGI(TAG, "PMS5003: PM1=%u PM2.5=%u PM10=%u", pm1_0, pm2_5, pm10);
    } else {
        ESP_LOGW(TAG, "PMS5003: данные ещё не получены");
    }
    ESP_LOGI(TAG, "======================================");
}

void display_task(void* pvParameter)
{
    ESP_LOGI(TAG, "Инициализация ST7735...");
    st7735_init();
    st7735_fill_screen(ST7735_BLACK);
    draw_header();
    ESP_LOGI(TAG, "Дисплей готов");

    while (1) {
        update_values();
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}