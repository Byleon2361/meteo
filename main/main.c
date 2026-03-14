#include "adc.h"
#include "bmp280.h"
#include "dht22.h"
#include "display.h"
#include "driver/gpio.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "hal/adc_types.h"
#include "mq135.h"
#include "nvs_flash.h"
#include "relay.h"
#include "sensor_data.h"
#include "tunnel.h"
#include "webserver.h"
#include <stdint.h>
#include <string.h>

static const char* TAG = "MAIN";

#define DHT22_GPIO GPIO_NUM_4
#define RELAY_GPIO GPIO_NUM_27
#define ADC_CHANNEL ADC_CHANNEL_0
#define I2C_MASTER_SCL_IO 22
#define I2C_MASTER_SDA_IO 21
#define I2C_MASTER_FREQ_HZ 100000
#define I2C_MASTER_PORT I2C_NUM_0

#define WIFI_STA_SSID "TP-Link_D2CD"
#define WIFI_STA_PASSWORD "61629400"

#define WIFI_CONNECTED_BIT BIT0
static EventGroupHandle_t s_wifi_event_group;

static void
wifi_event_handler(void* arg, esp_event_base_t base, int32_t id, void* data)
{
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        wifi_event_sta_disconnected_t* event
                = (wifi_event_sta_disconnected_t*)data;
        ESP_LOGE(TAG, "WiFi отключён, причина: %d", event->reason);
        esp_wifi_connect();
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* e = (ip_event_got_ip_t*)data;
        ESP_LOGI(TAG, "Получен IP: " IPSTR, IP2STR(&e->ip_info.ip));
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

static void wifi_init_sta(void)
{
    s_wifi_event_group = xEventGroupCreate();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(
            WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(
            IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));
    wifi_config_t wifi_config = {
        .sta = {
            .ssid     = WIFI_STA_SSID,
            .password = WIFI_STA_PASSWORD,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    esp_wifi_connect();

    ESP_LOGI(TAG, "Ждём подключения к WiFi...");
    xEventGroupWaitBits(
            s_wifi_event_group, WIFI_CONNECTED_BIT, false, true, portMAX_DELAY);
    ESP_LOGI(TAG, "WiFi подключён");
}

static void i2c_master_init(void)
{
    i2c_config_t conf = {
            .mode = I2C_MODE_MASTER,
            .sda_io_num = I2C_MASTER_SDA_IO,
            .scl_io_num = I2C_MASTER_SCL_IO,
            .sda_pullup_en = GPIO_PULLUP_ENABLE,
            .scl_pullup_en = GPIO_PULLUP_ENABLE,
            .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    ESP_ERROR_CHECK(i2c_param_config(I2C_MASTER_PORT, &conf));
    ESP_ERROR_CHECK(i2c_driver_install(I2C_MASTER_PORT, conf.mode, 0, 0, 0));
}

void app_main(void)
{
    ESP_LOGI(TAG, "Инициализация метеостанции...");

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES
        || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    init_relay(RELAY_GPIO);
    i2c_master_init();
    ESP_LOGI(TAG, "I2C инициализирован");

    sensor_data_init();
    adc_init(ADC_CHANNEL);

    mq_params_data_t mq_params = {.channel = ADC_CHANNEL, .task_delay_s = 5};
    dht_params_data_t dht_params = {.gpio = DHT22_GPIO, .task_delay_s = 5};
    bmp_params_data_t bmp_params = {.port = I2C_MASTER_PORT, .task_delay_s = 5};

    xTaskCreatePinnedToCore(
            mq_sensor_task, "mq_sensor_task", 4096, &mq_params, 5, NULL, 0);
    xTaskCreatePinnedToCore(
            dht22_task, "dht22_task", 4096, &dht_params, 5, NULL, 1);
    xTaskCreatePinnedToCore(
            bmp280_task, "bmp280_task", 4096, &bmp_params, 5, NULL, 1);
    xTaskCreatePinnedToCore(
            display_task, "display_task", 4096, NULL, 4, NULL, 0);

    wifi_init_sta();
    start_webserver();
    tunnel_init();

    while (1) {
        vTaskDelay(portMAX_DELAY);
    }
}