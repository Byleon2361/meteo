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

#define W ST7735_WIDTH  // 128
#define H ST7735_HEIGHT // 160

#define MARGIN 4
#define ROW_H 14 // если не хватает места, то поменять это значение
#define LABEL_SCALE 1
#define VALUE_SCALE 2

static void draw_divider(int16_t y)
{
    st7735_draw_hline(0, y, W, ST7735_DARKGRAY);
}

static void
draw_row(int16_t y, const char* label, const char* value, uint16_t color)
{
    st7735_draw_string(
            MARGIN, y + 2, label, ST7735_GRAY, ST7735_BLACK, LABEL_SCALE);

    int16_t vx = W - (int16_t)strlen(value) * 6 * VALUE_SCALE - MARGIN;
    if (vx < 40)
        vx = 40;
    st7735_fill_rect(38, y, W - 38, ROW_H - 1, ST7735_BLACK);
    st7735_draw_string(vx, y, value, color, ST7735_BLACK, VALUE_SCALE);
}

static void draw_header(void)
{
    st7735_fill_rect(0, 0, W, 14, ST7735_BLUE);
    st7735_draw_string(
            MARGIN, 3, "ESP32 WEATHER", ST7735_WHITE, ST7735_BLUE, 1);
}

static void draw_static_labels(void)
{
    int16_t y = 16;

    st7735_draw_string(
            MARGIN, y, "TEMP", ST7735_CYAN, ST7735_BLACK, LABEL_SCALE);
    y += ROW_H;
    draw_divider(y - 2);
    st7735_draw_string(
            MARGIN, y, "HUM", ST7735_CYAN, ST7735_BLACK, LABEL_SCALE);
    y += ROW_H;
    draw_divider(y - 2);
    st7735_draw_string(
            MARGIN, y, "PRES", ST7735_CYAN, ST7735_BLACK, LABEL_SCALE);
    y += ROW_H;
    draw_divider(y - 2);

    st7735_fill_rect(0, y, W, 11, 0x0010);
    st7735_draw_string(
            MARGIN, y + 2, "-- GASES --", ST7735_GRAY, 0x0010, LABEL_SCALE);
    y += 12;

    st7735_draw_string(
            MARGIN, y, "CO2", ST7735_YELLOW, ST7735_BLACK, LABEL_SCALE);
    y += ROW_H;
    draw_divider(y - 2);
    st7735_draw_string(
            MARGIN, y, "CO", ST7735_ORANGE, ST7735_BLACK, LABEL_SCALE);
    y += ROW_H;
    draw_divider(y - 2);
    st7735_draw_string(
            MARGIN, y, "NH3", ST7735_ORANGE, ST7735_BLACK, LABEL_SCALE);
    y += ROW_H;
    draw_divider(y - 2);

    st7735_fill_rect(0, y, W, 11, 0x0810);
    st7735_draw_string(
            MARGIN, y + 2, "-- DUST --", ST7735_GRAY, 0x0810, LABEL_SCALE);
    y += 12;

    st7735_draw_string(
            MARGIN, y, "PM1", ST7735_GREEN, ST7735_BLACK, LABEL_SCALE);
    y += ROW_H;
    draw_divider(y - 2);
    st7735_draw_string(
            MARGIN, y, "PM2.5", ST7735_GREEN, ST7735_BLACK, LABEL_SCALE);
    y += ROW_H;
    draw_divider(y - 2);
    st7735_draw_string(
            MARGIN, y, "PM10", ST7735_GREEN, ST7735_BLACK, LABEL_SCALE);
}

static void update_values(void)
{
    char buf[20];
    int16_t y = 16;

    float temperature_dht, humidity;
    uint8_t dht_valid;
    sensor_data_get_dht(&temperature_dht, &humidity, &dht_valid);

    if (dht_valid == DHT_OK) {
        snprintf(buf, sizeof(buf), "%.1fC", temperature_dht);
        draw_row(y, "TEMP", buf, ST7735_WHITE);
    } else {
        draw_row(y, "TEMP", "ERR", ST7735_RED);
    }
    y += ROW_H;

    snprintf(buf, sizeof(buf), "%.1f%%", humidity);
    draw_row(y, "HUM", buf, ST7735_WHITE);
    y += ROW_H;

    float temperature_bmp, pressure;
    uint8_t bmp_valid;
    sensor_data_get_bmp(&temperature_bmp, &pressure, &bmp_valid);
    snprintf(buf, sizeof(buf), "%.0fhPa", pressure);
    draw_row(y, "PRES", buf, ST7735_WHITE);
    y += ROW_H;

    y += 12;

    float co2_ppm, lpg_ppm, co_ppm, nh3_ppm;
    sensor_data_get_mq(&co2_ppm, &lpg_ppm, &co_ppm, &nh3_ppm);

    uint16_t co2_color = co2_ppm > 1000 ? ST7735_RED
            : co2_ppm > 700             ? ST7735_YELLOW
                                        : ST7735_WHITE;
    snprintf(buf, sizeof(buf), "%.0fppm", co2_ppm);
    draw_row(y, "CO2", buf, co2_color);
    y += ROW_H;

    uint16_t co_color = co_ppm > 50 ? ST7735_RED : ST7735_WHITE;
    snprintf(buf, sizeof(buf), "%.1fppm", co_ppm);
    draw_row(y, "CO", buf, co_color);
    y += ROW_H;

    snprintf(buf, sizeof(buf), "%.1fppm", nh3_ppm);
    draw_row(y, "NH3", buf, ST7735_WHITE);
    y += ROW_H;

    y += 12;

    uint16_t pm1_0, pm2_5, pm10;
    uint8_t pms_valid;
    sensor_data_get_pms5003(&pm1_0, &pm2_5, &pm10, &pms_valid);

    if (pms_valid) {
        uint16_t pm_color = pm2_5 > 35 ? ST7735_RED
                : pm2_5 > 12           ? ST7735_YELLOW
                                       : ST7735_WHITE;
        snprintf(buf, sizeof(buf), "%u ug", pm1_0);
        draw_row(y, "PM1", buf, ST7735_WHITE);
        y += ROW_H;
        snprintf(buf, sizeof(buf), "%u ug", pm2_5);
        draw_row(y, "PM2.5", buf, pm_color);
        y += ROW_H;
        snprintf(buf, sizeof(buf), "%u ug", pm10);
        draw_row(y, "PM10", buf, ST7735_WHITE);
    } else {
        draw_row(y, "PM1", "wait..", ST7735_GRAY);
        y += ROW_H;
        draw_row(y, "PM2.5", "wait..", ST7735_GRAY);
        y += ROW_H;
        draw_row(y, "PM10", "wait..", ST7735_GRAY);
    }

    ESP_LOGI(TAG, "========== СЕНСОРНЫЕ ДАННЫЕ ==========");

    if (dht_valid == DHT_OK) {
        ESP_LOGI(
                TAG,
                "Температура: %.1f C, Влажность: %.1f%%",
                temperature_dht,
                humidity);
    } else if (dht_valid == DHT_TIMEOUT) {
        ESP_LOGW(TAG, "DHT22: timeout");
    } else if (dht_valid == DHT_CHECKSUM_FAIL) {
        ESP_LOGW(TAG, "DHT22: Ошибка подсчета контрольной суммы");
    } else {
        ESP_LOGW(TAG, "DHT22: Неизвестная ошибка");
    }

    ESP_LOGI(
            TAG,
            "Температура BMP: %.1f C, Давление: %.1f hPa",
            temperature_bmp,
            pressure);

    ESP_LOGI(TAG, "MQ-135:");
    ESP_LOGI(TAG, "  CO2: %.2f ppm", co2_ppm);
    ESP_LOGI(TAG, "  CO:  %.2f ppm", co_ppm);
    ESP_LOGI(TAG, "  NH3: %.2f ppm", nh3_ppm);
    ESP_LOGI(TAG, "  LPG: %.2f ppm", lpg_ppm);

    if (pms_valid) {
        ESP_LOGI(TAG, "PMS5003 (частицы):");
        ESP_LOGI(TAG, "  PM1.0: %u мкг/м3", pm1_0);
        ESP_LOGI(TAG, "  PM2.5: %u мкг/м3", pm2_5);
        ESP_LOGI(TAG, "  PM10:  %u мкг/м3", pm10);
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
    draw_static_labels();
    ESP_LOGI(TAG, "Дисплей готов");

    while (1) {
        update_values();
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}