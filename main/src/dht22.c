#include <stdint.h>
#include <stdio.h>

#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "dht22.h"
#include "sensor_data.h"

static const char* TAG = "DHT22";

static uint8_t dht22_read(dht22_t* dht) {
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

  uint64_t timeout =
      esp_timer_get_time() + 20000;
  while (gpio_get_level(dht->gpio) == 1)
    if (esp_timer_get_time() > timeout) return DHT_TIMEOUT;
  while (gpio_get_level(dht->gpio) == 0)
    if (esp_timer_get_time() > timeout) return DHT_TIMEOUT;
  while (gpio_get_level(dht->gpio) == 1)
    if (esp_timer_get_time() > timeout) return DHT_TIMEOUT;

  for (int i = 0; i < 40; i++) {
    // Ждём переход 0→1 (начало бита)
    while (gpio_get_level(dht->gpio) == 0)
      if (esp_timer_get_time() > timeout) return DHT_TIMEOUT;
    uint64_t high_start = esp_timer_get_time();

    // Ждём переход 1→0 (конец бита)
    while (gpio_get_level(dht->gpio) == 1)
      if (esp_timer_get_time() > timeout) return DHT_TIMEOUT;

    // Если HIGH был дольше ~50 мкс → бит = 1
    if (esp_timer_get_time() - high_start > 50) 
    {
      dht->data[i / 8] |= (1 << (7 - (i % 8)));
    }
  }

  uint8_t checksum = dht->data[0] + dht->data[1] + dht->data[2] + dht->data[3];
  if (dht->data[4] != checksum) 
  {
    return DHT_CHECKSUM_FAIL;
  }

  dht->last_read_time = esp_timer_get_time();
  return DHT_OK;
}

void dht22_task(void *pvParameter)
{
    dht_params_data_t *params = (dht_params_data_t*)pvParameter;
    ESP_LOGI(TAG, "Запуск DHT22 на GPIO%d", params->gpio);

    dht22_t dht = {.gpio = params->gpio, .last_read_time = 0};
    gpio_reset_pin(dht.gpio);
    gpio_set_pull_mode(dht.gpio, GPIO_PULLUP_ONLY);

    vTaskDelay(pdMS_TO_TICKS(2500));

    while (1) {
        uint8_t result = dht22_read(&dht);

        if (result == DHT_OK) {
            // Преобразование байтов в значения
            float humidity = dht.data[0] + dht.data[1] / 10.0f;
            float temperature = dht.data[2] + dht.data[3] / 10.0f;
            
            // Запись в общую структуру
            sensor_data_set_dht(temperature, humidity);
            
            // Логирование (опционально)
            ESP_LOGI(TAG, "DHT22: %.1f°C, %.1f%%", temperature, humidity);
        } else {
            ESP_LOGW(TAG, "Ошибка чтения DHT22: %d", result);
        }

        vTaskDelay(pdMS_TO_TICKS(2500));
    }
}