#include "sensor_data.h"
#include "esp_log.h"

static const char* TAG = "SENSOR_DATA";
static sensor_data_t g_sensor_data;

void sensor_data_init(void) {
    g_sensor_data.mutex = xSemaphoreCreateMutex();
    if (g_sensor_data.mutex == NULL) {
        ESP_LOGE(TAG, "Не удалось создать мьютекс");
    }
    
    // Инициализация данных
    g_sensor_data.dht_valid = false;
    g_sensor_data.mq_valid = false;
}

void sensor_data_set_dht(float temperature, float humidity) {
    if (xSemaphoreTake(g_sensor_data.mutex, portMAX_DELAY) == pdTRUE) {
        g_sensor_data.temperature = temperature;
        g_sensor_data.humidity = humidity;
        g_sensor_data.dht_valid = true;
        xSemaphoreGive(g_sensor_data.mutex);
    }
}

void sensor_data_set_mq(int raw_adc, float voltage, float rs_ro_ratio,
                       float co2, float lpg, float co, float nh3) {
    if (xSemaphoreTake(g_sensor_data.mutex, portMAX_DELAY) == pdTRUE) {
        g_sensor_data.mq_raw_adc = raw_adc;
        g_sensor_data.mq_voltage = voltage;
        g_sensor_data.mq_rs_ro_ratio = rs_ro_ratio;
        g_sensor_data.co2_ppm = co2;
        g_sensor_data.lpg_ppm = lpg;
        g_sensor_data.co_ppm = co;
        g_sensor_data.nh3_ppm = nh3;
        g_sensor_data.mq_valid = true;
        xSemaphoreGive(g_sensor_data.mutex);
    }
}

void sensor_data_get_dht(float *temperature, float *humidity, bool *valid) {
    if (xSemaphoreTake(g_sensor_data.mutex, portMAX_DELAY) == pdTRUE) {
        *temperature = g_sensor_data.temperature;
        *humidity = g_sensor_data.humidity;
        *valid = g_sensor_data.dht_valid;
        xSemaphoreGive(g_sensor_data.mutex);
    }
}

void sensor_data_get_mq(float *co2, float *lpg, float *co, float *nh3, bool *valid) {
    if (xSemaphoreTake(g_sensor_data.mutex, portMAX_DELAY) == pdTRUE) {
        *co2 = g_sensor_data.co2_ppm;
        *lpg = g_sensor_data.lpg_ppm;
        *co = g_sensor_data.co_ppm;
        *nh3 = g_sensor_data.nh3_ppm;
        *valid = g_sensor_data.mq_valid;
        xSemaphoreGive(g_sensor_data.mutex);
    }
}