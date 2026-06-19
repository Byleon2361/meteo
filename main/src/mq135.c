#include <math.h>

#include "adc.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hal/adc_types.h"
#include "mq135.h"
#include "sensor_data.h"

static const char* TAG = "MQ_SENSOR";

static float calculate_rs(float vrl)
{
    float vcc = 3.3;
    return ((vcc - vrl) / vrl) * RL_VALUE;
}
static float read_adc_voltage_avg(adc_channel_t channel, int samples)
{
    float sum = 0;
    for (int i = 0; i < samples; i++) {
        sum += read_adc_voltage(channel);
        vTaskDelay(pdMS_TO_TICKS(20));
    }
    return sum / samples;
}

static float calculate_rs_ro_ratio(adc_channel_t channel, float ro)
{
    float voltage = read_adc_voltage_avg(channel, 10);   // ← было одно измерение
    float rs = calculate_rs(voltage);
    return rs / ro;
}

static float clamp_ppm(float value, float min_v, float max_v)
{
    if (isnan(value) || isinf(value)) return min_v;
    if (value < min_v) return min_v;
    if (value > max_v) return max_v;
    return value;
}

static float calculate_co2_ppm(float rs_ro_ratio)
{
    float a = -0.352519, b = 5.17901;
    float ppm = powf(rs_ro_ratio / b, 1.0f / a);
    return clamp_ppm(ppm, 400.0f, 5000.0f);   // CO2 в воздухе не бывает < 400
}

static float calculate_co_ppm(float rs_ro_ratio)
{
    float a = -0.229115, b = 4.98267;
    float ppm = powf(rs_ro_ratio / b, 1.0f / a);
    return clamp_ppm(ppm, 0.0f, 1000.0f);   // выше 1000 ppm — почти наверняка шум/ошибка
}

static float calculate_nh3_ppm(float rs_ro_ratio)
{
    float a = -0.41162, b = 6.6564;
    float ppm = powf(rs_ro_ratio / b, 1.0f / a);
    return clamp_ppm(ppm, 0.0f, 300.0f);
}

static float calculate_lpg_ppm(float rs_ro_ratio)
{
    float a = -0.30219, b = 3.00802;
    float ppm = powf(rs_ro_ratio / b, 1.0f / a);
    return clamp_ppm(ppm, 0.0f, 1000.0f);
}

void mq_sensor_task(void* mq_params)
{
    mq_params_data_t* params = (mq_params_data_t*)mq_params;

    if (params == NULL) {
        ESP_LOGE(TAG, "Параметры датчика не переданы!");
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "Задача датчика MQ запущена");
    ESP_LOGI(TAG, "Канал ADC: %d", params->channel);

    while (1) {
        int raw_adc = read_adc_raw(params->channel);
        float voltage = read_adc_voltage(params->channel);
        float rs_ro_ratio = calculate_rs_ro_ratio(params->channel, RO_VALUE);
        ESP_LOGI(TAG, "rs_ro_ratio %.2f", rs_ro_ratio);
        float co2_ppm = calculate_co2_ppm(rs_ro_ratio);
        float co_ppm = calculate_co_ppm(rs_ro_ratio);
        float nh3_ppm = calculate_nh3_ppm(rs_ro_ratio);
        float lpg_ppm = calculate_lpg_ppm(rs_ro_ratio);

        sensor_data_set_mq(
                raw_adc,
                voltage,
                rs_ro_ratio,
                co2_ppm,
                lpg_ppm,
                co_ppm,
                nh3_ppm);

        vTaskDelay(pdMS_TO_TICKS(params->task_delay_s * 1000));
    }
}
