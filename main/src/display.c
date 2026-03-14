#include "display.h"
#include "dht22.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "mq135.h"
#include "pms5003.h"
#include "sensor_data.h"
#include <stdint.h>

static const char* TAG = "DISPLAY";

void display_task(void* pvParameter)
{
    ESP_LOGI(TAG, "Задача отображения запущена");
    while (1) {
        float temperature_dht, humidity, temperature_bmp, pressure;
        uint8_t dht_valid, bmp_valid;
        sensor_data_get_dht(&temperature_dht, &humidity, &dht_valid);
        sensor_data_get_bmp(&temperature_bmp, &pressure, &bmp_valid);

        float co2_ppm, lpg_ppm, co_ppm, nh3_ppm;
        sensor_data_get_mq(&co2_ppm, &lpg_ppm, &co_ppm, &nh3_ppm);

        uint16_t pm1_0, pm2_5, pm10;
        uint8_t pms_valid;
        sensor_data_get_pms5003(&pm1_0, &pm2_5, &pm10, &pms_valid);

        ESP_LOGI(TAG, "========== СЕНСОРНЫЕ ДАННЫЕ ==========");

        if (dht_valid == DHT_OK) {
            ESP_LOGI(
                    TAG,
                    "Температура: %.1f°C, Влажность: %.1f%%",
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
                "Температура: %.1f°C, Давление: %.1f mmHg",
                temperature_bmp,
                pressure);

        ESP_LOGI(TAG, "MQ-135:");
        ESP_LOGI(TAG, "  CO2: %.2f ppm", co2_ppm);
        ESP_LOGI(TAG, "  CO:  %.2f ppm", co_ppm);
        ESP_LOGI(TAG, "  NH3: %.2f ppm", nh3_ppm);
        ESP_LOGI(TAG, "  LPG: %.2f ppm", lpg_ppm);

        if (pms_valid) {
            ESP_LOGI(TAG, "PMS5003 (частицы):");
            ESP_LOGI(TAG, "  PM1.0: %u мкг/м³", pm1_0);
            ESP_LOGI(TAG, "  PM2.5: %u мкг/м³", pm2_5);
            ESP_LOGI(TAG, "  PM10:  %u мкг/м³", pm10);
        } else {
            ESP_LOGW(TAG, "PMS5003: данные ещё не получены");
        }

        ESP_LOGI(TAG, "======================================");

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}