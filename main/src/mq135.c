#include <math.h>

#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hal/adc_types.h"
#include "adc.h"
#include "mq135.h"
#include "sensor_data.h"

static const char* TAG = "MQ_SENSOR";

// Расчет сопротивления датчика
static float calculate_rs(float vrl) {
  float vcc = 3.3;  // Напряжение питания
  return ((vcc - vrl) / vrl) * RL_VALUE;
}

// Расчет отношения Rs/Ro
static float calculate_rs_ro_ratio(adc_channel_t channel, float ro) {
  float voltage = read_adc_voltage(channel);
  float rs = calculate_rs(voltage);
  return rs / ro;
}

// Пример расчета PPM для MQ-135 (CO2)
static float calculate_co2_ppm(float rs_ro_ratio) {
  // Коэффициенты для кривой MQ-135 (CO2)
  float a = 116.6020682;
  float b = -2.769034857;

  return a * powf(rs_ro_ratio, b);
}

// Пример расчета PPM для MQ-2 (LPG)
static float calculate_lpg_ppm(float rs_ro_ratio) {
  // Коэффициенты для кривой MQ-2 (LPG)
  float a = 574.25;
  float b = -2.222;

  return a * powf(rs_ro_ratio, b);
}

// Пример расчета PPM для MQ-7 (CO)
static float calculate_co_ppm(float rs_ro_ratio) {
  // Коэффициенты для кривой MQ-7 (CO)
  float a = 99.042;
  float b = -1.518;

  return a * powf(rs_ro_ratio, b);
}

// Пример расчета PPM для MQ-135 (NH3/аммиак)
static float calculate_nh3_ppm(float rs_ro_ratio) {
  // Коэффициенты для кривой MQ-135 (NH3)
  float a = 102.2;
  float b = -2.473;

  return a * powf(rs_ro_ratio, b);
}

void mq_sensor_task(void* pvParameter) 
{
    mq_params_data_t *params = (mq_params_data_t*)pvParameter;
    
    if (params == NULL) {
        ESP_LOGE(TAG, "Параметры датчика не переданы!");
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "Задача датчика MQ запущена");
    ESP_LOGI(TAG, "Канал ADC: %d", params->channel);

    while (1) {
        // Чтение данных
        int raw_adc = read_adc_raw(params->channel);
        float voltage = read_adc_voltage(params->channel);
        float rs_ro_ratio = calculate_rs_ro_ratio(params->channel, RO_VALUE);
        float co2_ppm = calculate_co2_ppm(rs_ro_ratio);
        float lpg_ppm = calculate_lpg_ppm(rs_ro_ratio);
        float co_ppm = calculate_co_ppm(rs_ro_ratio);
        float nh3_ppm = calculate_nh3_ppm(rs_ro_ratio);

        // Запись в общую структуру
        sensor_data_set_mq(raw_adc, voltage, rs_ro_ratio, 
                          co2_ppm, lpg_ppm, co_ppm, nh3_ppm);

        // Логирование (опционально)
        ESP_LOGI(TAG, "MQ-135: ADC=%d, %.3fV, Rs/Ro=%.2f, CO2=%.2fppm", 
                raw_adc, voltage, rs_ro_ratio, co2_ppm);

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}