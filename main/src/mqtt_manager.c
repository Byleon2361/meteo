#include "mqtt_manager.h"
#include "sensor_data.h"
#include "mqtt_client.h"   // esp-mqtt из ESP-IDF
#include "esp_log.h"
#include <stdio.h>
#include <string.h>

static const char *TAG = "MQTT_MGR";

// ── Настройки (вынеси в config.h) ─────────────────────────
#define MQTT_BROKER_URI  "mqtt://72.56.93.56:1883"
#define MQTT_USERNAME    "meteo"
#define MQTT_PASSWORD    "xvtZQo-5GiCaDX"
#define MQTT_CLIENT_ID   "esp32_meteo"
// ──────────────────────────────────────────────────────────

static esp_mqtt_client_handle_t s_client = NULL;
static bool                     s_connected = false;

// Обработчик событий MQTT
static void mqtt_event_handler(void *arg,
                                esp_event_base_t base,
                                int32_t event_id,
                                void *event_data)
{
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;

    switch (event->event_id) {

    case MQTT_EVENT_CONNECTED:
        s_connected = true;
        ESP_LOGI(TAG, "Подключён к брокеру");

        // LWT «online»
        esp_mqtt_client_publish(s_client,
            "home/sensors/status", "online", 0, 1, true);

        // Подписываемся на управляющие команды
        esp_mqtt_client_subscribe(s_client, "home/fan/set",        1);
        esp_mqtt_client_subscribe(s_client, "home/thresholds/pm25",1);
        break;

    case MQTT_EVENT_DISCONNECTED:
        s_connected = false;
        ESP_LOGW(TAG, "Отключён от брокера, переподключение...");
        break;

    case MQTT_EVENT_DATA:
        // Обрабатываем входящие команды
        ESP_LOGI(TAG, "Команда: топик=%.*s  данные=%.*s",
                 event->topic_len,   event->topic,
                 event->data_len,    event->data);
        break;

    case MQTT_EVENT_ERROR:
        ESP_LOGE(TAG, "Ошибка MQTT");
        break;

    default:
        break;
    }
}

esp_err_t mqtt_manager_init(void)
{
    esp_mqtt_client_config_t cfg = {
        .broker.address.uri          = MQTT_BROKER_URI,
        .credentials.username        = MQTT_USERNAME,
        .credentials.authentication.password = MQTT_PASSWORD,
        .credentials.client_id       = MQTT_CLIENT_ID,
        // LWT — если ESP32 пропадёт, брокер сам отправит "offline"
        .session.last_will.topic     = "home/sensors/status",
        .session.last_will.msg       = "offline",
        .session.last_will.qos       = 1,
        .session.last_will.retain    = true,
        // Автоматическое переподключение
        .network.reconnect_timeout_ms = 10000,
    };

    s_client = esp_mqtt_client_init(&cfg);
    if (!s_client) return ESP_FAIL;

    esp_mqtt_client_register_event(s_client,
        ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);

    return esp_mqtt_client_start(s_client);
}

bool mqtt_manager_is_connected(void) { return s_connected; }

// Публикует {"value": 23.4, "unit": "C"}
esp_err_t mqtt_publish_float(const char *topic,
                              const char *key,
                              float value)
{
    if (!s_connected) return ESP_ERR_INVALID_STATE;
    char buf[64];
    snprintf(buf, sizeof(buf), "{\"%s\": %.2f}", key, value);
    int msg_id = esp_mqtt_client_publish(s_client, topic, buf, 0, 0, false);
    return (msg_id >= 0) ? ESP_OK : ESP_FAIL;
}

// {"pm1_0": 5, "pm2_5": 12, "pm10": 18}
esp_err_t mqtt_publish_pm(uint16_t pm1_0, uint16_t pm2_5, uint16_t pm10)
{
    if (!s_connected) return ESP_ERR_INVALID_STATE;
    char buf[80];
    snprintf(buf, sizeof(buf),
             "{\"pm1_0\":%u,\"pm2_5\":%u,\"pm10\":%u}",
             pm1_0, pm2_5, pm10);
    int id = esp_mqtt_client_publish(s_client,
                 "home/sensors/pm25", buf, 0, 0, false);
    return (id >= 0) ? ESP_OK : ESP_FAIL;
}

// {"co2": 412.0, "co": 1.2, "nh3": 0.8, "lpg": 0.5}
esp_err_t mqtt_publish_gases(float co2, float co, float nh3, float lpg)
{
    if (!s_connected) return ESP_ERR_INVALID_STATE;
    char buf[128];
    snprintf(buf, sizeof(buf),
             "{\"co2\":%.1f,\"co\":%.2f,\"nh3\":%.2f,\"lpg\":%.2f}",
             co2, co, nh3, lpg);
    int id = esp_mqtt_client_publish(s_client,
                 "home/sensors/air_quality", buf, 0, 0, false);
    return (id >= 0) ? ESP_OK : ESP_FAIL;
}

// Собирает все данные из sensor_data и публикует разом
esp_err_t mqtt_publish_all(void)
{
    if (!s_connected) {
        ESP_LOGW(TAG, "MQTT не подключён, пропускаем публикацию");
        return ESP_ERR_INVALID_STATE;
    }

    float temp_dht, humidity; uint8_t dht_valid;
    sensor_data_get_dht(&temp_dht, &humidity, &dht_valid);

    float temp_bmp, pressure; uint8_t bmp_valid;
    sensor_data_get_bmp(&temp_bmp, &pressure, &bmp_valid);

    float co2, lpg, co, nh3;
    sensor_data_get_mq(&co2, &lpg, &co, &nh3);

    uint16_t pm1_0, pm2_5, pm10; uint8_t pms_valid;
    sensor_data_get_pms5003(&pm1_0, &pm2_5, &pm10, &pms_valid);

    // Публикуем каждый параметр
    mqtt_publish_float("home/sensors/temperature", "value", temp_dht);
    mqtt_publish_float("home/sensors/humidity",    "value", humidity);
    mqtt_publish_float("home/sensors/pressure",    "value", pressure);
    mqtt_publish_float("home/sensors/co2",         "value", co2);

    if (pms_valid) {
        mqtt_publish_pm(pm1_0, pm2_5, pm10);
    }
    mqtt_publish_gases(co2, co, nh3, lpg);

    ESP_LOGI(TAG, "Все данные опубликованы в MQTT");
    return ESP_OK;
}
void mqtt_publish_task(void *arg)
{
    // Ждём первого подключения к MQTT
    vTaskDelay(pdMS_TO_TICKS(5000));

    while (1) {
        mqtt_publish_all();
        vTaskDelay(pdMS_TO_TICKS(300000)); // 5 минут
    }
}