#include "dht22.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sensor_data.h"
#include <string.h>

static const char* TAG = "DHT22";
static portMUX_TYPE dht_mux = portMUX_INITIALIZER_UNLOCKED;

static uint8_t dht22_read(dht22_t* dht)
{
    uint64_t start_time = esp_timer_get_time();
    if (start_time - dht->last_read_time < 2000000) {
        return DHT_TIMEOUT;
    }
    memset(dht->data, 0, 5);

    gpio_set_direction(dht->gpio, GPIO_MODE_OUTPUT);
    gpio_set_level(dht->gpio, 0);
    vTaskDelay(pdMS_TO_TICKS(20));
    gpio_set_level(dht->gpio, 1);
    esp_rom_delay_us(30);
    gpio_set_direction(dht->gpio, GPIO_MODE_INPUT);
    gpio_pullup_en(dht->gpio);

    taskENTER_CRITICAL(&dht_mux);

    uint64_t timeout = esp_timer_get_time() + 20000;
    uint8_t err = 0;

    while (gpio_get_level(dht->gpio) == 1)
        if (esp_timer_get_time() > timeout) { err = 1; break; }
    if (!err)
        while (gpio_get_level(dht->gpio) == 0)
            if (esp_timer_get_time() > timeout) { err = 1; break; }
    if (!err)
        while (gpio_get_level(dht->gpio) == 1)
            if (esp_timer_get_time() > timeout) { err = 1; break; }

    if (!err) {
        for (int i = 0; i < 40; i++) {
            while (gpio_get_level(dht->gpio) == 0)
                if (esp_timer_get_time() > timeout) { err = 1; break; }
            if (err) break;
            uint64_t high_start = esp_timer_get_time();
            while (gpio_get_level(dht->gpio) == 1)
                if (esp_timer_get_time() > timeout) { err = 1; break; }
            if (err) break;
            if (esp_timer_get_time() - high_start > 50) {
                dht->data[i / 8] |= (1 << (7 - (i % 8)));
            }
        }
    }

    taskEXIT_CRITICAL(&dht_mux);

    if (err) {
        return DHT_TIMEOUT;
    }

    uint8_t checksum
            = dht->data[0] + dht->data[1] + dht->data[2] + dht->data[3];
    if (dht->data[4] != checksum) {
        return DHT_CHECKSUM_FAIL;
    }
    dht->last_read_time = esp_timer_get_time();
    return DHT_OK;
}

void dht22_task(void* dht_params)
{
    dht_params_data_t* params = (dht_params_data_t*)dht_params;
    ESP_LOGI(TAG, "Запуск DHT22 на GPIO%d", params->gpio);

    dht22_t dht = {.gpio = params->gpio, .last_read_time = 0};
    gpio_reset_pin(dht.gpio);
    gpio_set_pull_mode(dht.gpio, GPIO_PULLUP_ONLY);
    vTaskDelay(pdMS_TO_TICKS(2500));

    while (1) {
        uint8_t result = DHT_TIMEOUT;

        for (int attempt = 0; attempt < 3; attempt++) {
            dht.last_read_time = 0;
            result = dht22_read(&dht);
            if (result == DHT_OK)
                break;
            vTaskDelay(pdMS_TO_TICKS(50));
        }

        if (result == DHT_OK) {
            float humidity = dht.data[0] + dht.data[1] / 10.0f;
            float temperature = dht.data[2] + dht.data[3] / 10.0f;

            ESP_LOGW(TAG, "RAW: %02X %02X %02X %02X %02X",
                     dht.data[0], dht.data[1], dht.data[2],
                     dht.data[3], dht.data[4]);

            if (humidity >= 0.0f && humidity <= 100.0f
                && temperature >= -40.0f && temperature <= 80.0f) {
                ESP_LOGI(TAG, "T=%.1f H=%.1f", temperature, humidity);
                sensor_data_set_dht(temperature, humidity, 1);
            } else {
                ESP_LOGW(TAG, "Вне диапазона: t=%.1f h=%.1f",
                         temperature, humidity);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(2500));
    }
}