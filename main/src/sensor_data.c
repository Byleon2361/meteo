#include "sensor_data.h"
#include "esp_log.h"
#include <stdint.h>

static const char* TAG = "SENSOR_DATA";
static sensor_data_t sensor_data;

void sensor_data_init(void)
{
    sensor_data.mutex = xSemaphoreCreateMutex();
    if (sensor_data.mutex == NULL) {
        ESP_LOGE(TAG, "Не удалось создать мьютекс");
    }

    sensor_data.dht_valid = 0;
}

void sensor_data_set_dht(float temperature_dht, float humidity, uint8_t valid)
{
    if (xSemaphoreTake(sensor_data.mutex, portMAX_DELAY) == pdTRUE) {
        sensor_data.temperature_dht = temperature_dht;
        sensor_data.humidity = humidity;
        sensor_data.dht_valid = valid;
        xSemaphoreGive(sensor_data.mutex);
    }
}

void sensor_data_get_dht(
        float* temperature_dht, float* humidity, uint8_t* valid)
{
    if (xSemaphoreTake(sensor_data.mutex, portMAX_DELAY) == pdTRUE) {
        *temperature_dht = sensor_data.temperature_dht;
        *humidity = sensor_data.humidity;
        *valid = sensor_data.dht_valid;
        xSemaphoreGive(sensor_data.mutex);
    }
}

void sensor_data_set_bmp(float temperature_bmp, float pressure, uint8_t valid)
{
    if (xSemaphoreTake(sensor_data.mutex, portMAX_DELAY) == pdTRUE) {
        sensor_data.temperature_bmp = temperature_bmp;
        sensor_data.pressure = pressure;
        sensor_data.bmp_valid = valid;
        xSemaphoreGive(sensor_data.mutex);
    }
}

void sensor_data_get_bmp(
        float* temperature_bmp, float* pressure, uint8_t* valid)
{
    if (xSemaphoreTake(sensor_data.mutex, portMAX_DELAY) == pdTRUE) {
        *temperature_bmp = sensor_data.temperature_bmp;
        *pressure = sensor_data.pressure;
        *valid = sensor_data.bmp_valid;
        xSemaphoreGive(sensor_data.mutex);
    }
}

void sensor_data_set_mq(
        int raw_adc,
        float voltage,
        float rs_ro_ratio,
        float co2,
        float lpg,
        float co,
        float nh3)
{
    if (xSemaphoreTake(sensor_data.mutex, portMAX_DELAY) == pdTRUE) {
        sensor_data.mq_raw_adc = raw_adc;
        sensor_data.mq_voltage = voltage;
        sensor_data.mq_rs_ro_ratio = rs_ro_ratio;
        sensor_data.co2_ppm = co2;
        sensor_data.lpg_ppm = lpg;
        sensor_data.co_ppm = co;
        sensor_data.nh3_ppm = nh3;
        xSemaphoreGive(sensor_data.mutex);
    }
}

void sensor_data_get_mq(float* co2, float* lpg, float* co, float* nh3)
{
    if (xSemaphoreTake(sensor_data.mutex, portMAX_DELAY) == pdTRUE) {
        *co2 = sensor_data.co2_ppm;
        *lpg = sensor_data.lpg_ppm;
        *co = sensor_data.co_ppm;
        *nh3 = sensor_data.nh3_ppm;
        xSemaphoreGive(sensor_data.mutex);
    }
}
