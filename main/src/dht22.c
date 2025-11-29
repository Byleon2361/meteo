#include <stdio.h>

#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char* TAG = "DHT22";

// =================== БИБЛИОТЕКА DHT22 ===================
#define DHT_OK 0
#define DHT_TIMEOUT 1
#define DHT_CHECKSUM_FAIL 2

typedef struct {
  uint8_t gpio;
  uint64_t last_read_time;
  uint8_t data[5];
} dht22_t;

static uint8_t dht22_read(dht22_t* dht) {
  uint64_t start_time = esp_timer_get_time();

  // Защита от слишком частых запросов (мин. 2 секунды)
  if (start_time - dht->last_read_time < 2000000) {
    return DHT_TIMEOUT;
  }

  memset(dht->data, 0, 5);

  // === Старт-сигнал ===
  gpio_set_direction(dht->gpio, GPIO_MODE_OUTPUT);
  gpio_set_level(dht->gpio, 0);
  vTaskDelay(pdMS_TO_TICKS(20));  // ≥18 мс LOW
  gpio_set_level(dht->gpio, 1);
  esp_rom_delay_us(30);  // 20–40 мкс HIGH
  gpio_set_direction(dht->gpio, GPIO_MODE_INPUT);
  gpio_pullup_en(dht->gpio);

  // Ждём ответ датчика: 80 мкс LOW + 80 мкс HIGH
  uint64_t timeout =
      esp_timer_get_time() + 20000;  // 20 мс общий таймаут на ответ
  while (gpio_get_level(dht->gpio) == 1)
    if (esp_timer_get_time() > timeout) return DHT_TIMEOUT;
  while (gpio_get_level(dht->gpio) == 0)
    if (esp_timer_get_time() > timeout) return DHT_TIMEOUT;
  while (gpio_get_level(dht->gpio) == 1)
    if (esp_timer_get_time() > timeout) return DHT_TIMEOUT;

  // Читаем 40 бит
  for (int i = 0; i < 40; i++) {
    // Ждём переход 0→1 (начало бита)
    while (gpio_get_level(dht->gpio) == 0)
      if (esp_timer_get_time() > timeout) return DHT_TIMEOUT;
    uint64_t high_start = esp_timer_get_time();

    // Ждём переход 1→0 (конец бита)
    while (gpio_get_level(dht->gpio) == 1)
      if (esp_timer_get_time() > timeout) return DHT_TIMEOUT;

    // Если HIGH был дольше ~50 мкс → бит = 1
    if (esp_timer_get_time() - high_start > 50) {
      dht->data[i / 8] |= (1 << (7 - (i % 8)));
    }
  }

  // Проверка контрольной суммы
  uint8_t checksum = dht->data[0] + dht->data[1] + dht->data[2] + dht->data[3];
  if (dht->data[4] != checksum) {
    return DHT_CHECKSUM_FAIL;
  }

  dht->last_read_time = esp_timer_get_time();
  return DHT_OK;
}

// =================== APP_MAIN ===================
void app_main(void) {
  ESP_LOGI(TAG, "Запуск DHT22 на GPIO%d", 18);

  dht22_t dht = {.gpio = 18, .last_read_time = 0};

  gpio_reset_pin(dht.gpio);
  gpio_set_pull_mode(dht.gpio, GPIO_PULLUP_ONLY);

  // Ждём стабилизации питания датчика
  vTaskDelay(pdMS_TO_TICKS(2500));

  while (1) {
    uint8_t result = dht22_read(&dht);

    if (result == DHT_OK) {
      // --- Сырые данные ---
      ESP_LOGI(TAG, "RAW bytes: %02X %02X %02X %02X %02X  (checksum %02X)",
               dht.data[0], dht.data[1], dht.data[2], dht.data[3], dht.data[4],
               dht.data[0] + dht.data[1] + dht.data[2] + dht.data[3]);

      ESP_LOGI(TAG, "RAW dec  : %3d %3d %3d %3d %3d", dht.data[0], dht.data[1],
               dht.data[2], dht.data[3], dht.data[4]);

      // --- Обработанные значения ---
      //   int16_t temp_raw = (dht.data[2] << 8) | dht.data[3];
      //   float temperature = (temp_raw & 0x8000) ? -((temp_raw & 0x7FFF)
      //   / 10.0f)
      //                                           : (temp_raw / 10.0f);

      //   float humidity = ((dht.data[0] << 8) | dht.data[1]) / 10.0f;

      //   ESP_LOGI(TAG, "Температура: %.1f °C    Влажность: %.1f %%",
      //   temperature,
      //            humidity);
      ESP_LOGI(TAG, "Температура: %d.%d °C    Влажность: %d.%d %%", dht.data[2],
               dht.data[3], dht.data[0], dht.data[1]);
    } else if (result == DHT_TIMEOUT) {
      ESP_LOGE(TAG, "Таймаут — датчик не отвечает или подключён неправильно");
    } else if (result == DHT_CHECKSUM_FAIL) {
      ESP_LOGE(TAG, "Ошибка контрольной суммы!");
      // Всё равно покажем сырые данные для отладки
      ESP_LOGW(TAG, "Некорректные RAW: %02X %02X %02X %02X %02X", dht.data[0],
               dht.data[1], dht.data[2], dht.data[3], dht.data[4]);
    }

    vTaskDelay(pdMS_TO_TICKS(
        2500));  // DHT22 рекомендуется опрашивать не чаще 1 раза в 2 секунды
  }
}
