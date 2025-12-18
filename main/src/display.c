#include "display.h"
#include "sensor_data.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char* TAG = "DISPLAY";

void display_task(void *pvParameter) {
    ESP_LOGI(TAG, "Задача отображения запущена");
    
    while (1) {
        float temperature, humidity;
        bool dht_valid;
        sensor_data_get_dht(&temperature, &humidity, &dht_valid);
        
        float co2_ppm, lpg_ppm, co_ppm, nh3_ppm;
        bool mq_valid;
        sensor_data_get_mq(&co2_ppm, &lpg_ppm, &co_ppm, &nh3_ppm, &mq_valid);
        
        ESP_LOGI(TAG, "========== СЕНСОРНЫЕ ДАННЫЕ ==========");
        
        if (dht_valid) {
            ESP_LOGI(TAG, "Температура: %.1f°C, Влажность: %.1f%%",temperature, humidity);
        } else {
            ESP_LOGW(TAG, "DHT22: данные недоступны");
        }
        
        if (mq_valid) {
            ESP_LOGI(TAG, "MQ-135:");
            ESP_LOGI(TAG, "  CO2: %.2f ppm", co2_ppm);
            ESP_LOGI(TAG, "  LPG: %.2f ppm", lpg_ppm);
            ESP_LOGI(TAG, "  CO: %.2f ppm", co_ppm);
            ESP_LOGI(TAG, "  NH3: %.2f ppm", nh3_ppm);
        } else {
            ESP_LOGW(TAG, "MQ-135: данные недоступны");
        }
        
        ESP_LOGI(TAG, "======================================");
        
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}