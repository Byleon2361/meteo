#include <stdint.h>
#include <string.h>

#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"
#include "hal/adc_types.h"
#include "nvs_flash.h"

#include "dht22.h"
#include "mq135.h"
#include "relay.h"
#include "adc.h"
#include "webserver.h"
#include "display.h"
#include "sensor_data.h"

static const char* TAG = "MAIN";

#define DHT22_GPIO GPIO_NUM_18 
#define RELAY_GPIO GPIO_NUM_27
#define ADC_CHANNEL ADC_CHANNEL_0 //GPIO36 для mq135

#define WIFI_AP_SSID "ESP32_METEO"
#define WIFI_AP_PASSWORD "12345678"
#define WIFI_AP_MAX_CONN 4

static void wifi_init_softap(void)
{
  esp_netif_create_default_wifi_ap();

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  wifi_config_t wifi_config = {
    .ap = {
      .ssid = WIFI_AP_SSID,
      .ssid_len = strlen(WIFI_AP_SSID),
      .password = WIFI_AP_PASSWORD,
      .max_connection = WIFI_AP_MAX_CONN,
      .authmode = WIFI_AUTH_WPA_WPA2_PSK},
  };

  if(strlen(WIFI_AP_PASSWORD) == 0)
    wifi_config.ap.authmode = WIFI_AUTH_OPEN;

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_start());

  ESP_LOGI(TAG, "WiFi AP started: SSID=%s", WIFI_AP_SSID);
}
void app_main(void) 
{
    ESP_LOGI(TAG, "Инициализация метеостанции...");
    
  esp_err_t ret = nvs_flash_init();
  if(ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
  {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ESP_ERROR_CHECK(nvs_flash_init());
  }

  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());

    init_relay(RELAY_GPIO);
    // Инициализация общей структуры данных
    sensor_data_init();
    ESP_LOGI(TAG, "Структура данных сенсоров инициализирована");
    
    // Инициализация ADC
    adc_init(ADC_CHANNEL);
    ESP_LOGI(TAG, "АЦП инициализирован");
    
    // Создание параметров для MQ-135
    mq_params_data_t mq_params = {
        .channel = ADC_CHANNEL
    };
    
    // Создание параметров для DHT22
    dht_params_data_t dht_params = {
        .gpio = DHT22_GPIO
    };
    
    // Запуск задач
    xTaskCreatePinnedToCore(mq_sensor_task, "mq_sensor_task", 4096, &mq_params, 5, NULL, 0);
    ESP_LOGI(TAG, "Задача MQ-135 создана");
    
    xTaskCreatePinnedToCore(dht22_task, "dht22_task", 4096, &dht_params, 5, NULL, 1);
    ESP_LOGI(TAG, "Задача DHT22 создана");
    
    // Запуск задачи отображения (на другом ядре для балансировки)
    xTaskCreatePinnedToCore(display_task, "display_task", 4096, NULL, 4, NULL, 0);
    ESP_LOGI(TAG, "Задача отображения создана");

    wifi_init_softap();
    start_webserver();
    
    // Основной цикл ничего не делает - все задачи работают независимо
    while (1) {
        vTaskDelay(portMAX_DELAY);
    }
}