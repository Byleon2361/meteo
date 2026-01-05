#include <math.h>

#include "adc.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hal/adc_types.h"
#include "mq135.h"
#include "sensor_data.h"

static const char *TAG = "MQ_SENSOR";

static float calculate_rs(float vrl)
{
  float vcc = 3.3;
  return ((vcc - vrl) / vrl) * RL_VALUE;
}

static float calculate_rs_ro_ratio(adc_channel_t channel, float ro)
{
  float voltage = read_adc_voltage(channel);
  float rs = calculate_rs(voltage);
  return rs / ro;
}

static float calculate_co2_ppm(float rs_ro_ratio)
{
  float a = -0.352519;
  float b = 5.17901;

  return powf(rs_ro_ratio / b, 1.0f / a);
}

static float calculate_co_ppm(float rs_ro_ratio)
{
  float a = -0.229115;
  float b = 4.98267;

  return powf(rs_ro_ratio / b, 1.0f / a);
}

static float calculate_nh3_ppm(float rs_ro_ratio)
{
  float a = -0.41162;
  float b = 6.6564;

  return powf(rs_ro_ratio / b, 1.0f / a);
}

static float calculate_lpg_ppm(float rs_ro_ratio)
{
  float a = -0.30219;
  float b = 3.00802;

  return powf(rs_ro_ratio / b, 1.0f / a);
}

void mq_sensor_task(void *mq_params)
{
  mq_params_data_t *params = (mq_params_data_t *)mq_params;

  if (params == NULL)
  {
    ESP_LOGE(TAG, "Параметры датчика не переданы!");
    vTaskDelete(NULL);
    return;
  }

  ESP_LOGI(TAG, "Задача датчика MQ запущена");
  ESP_LOGI(TAG, "Канал ADC: %d", params->channel);

  while (1)
  {
    int raw_adc = read_adc_raw(params->channel);
    float voltage = read_adc_voltage(params->channel);
    float rs_ro_ratio = calculate_rs_ro_ratio(params->channel, RO_VALUE);
    ESP_LOGI(TAG, "rs_ro_ratio %.2f", rs_ro_ratio);
    float co2_ppm = calculate_co2_ppm(rs_ro_ratio);
    float co_ppm = calculate_co_ppm(rs_ro_ratio);
    float nh3_ppm = calculate_nh3_ppm(rs_ro_ratio);
    float lpg_ppm = calculate_lpg_ppm(rs_ro_ratio);

    sensor_data_set_mq(raw_adc, voltage, rs_ro_ratio, co2_ppm, lpg_ppm, co_ppm,
                       nh3_ppm);

    vTaskDelay(pdMS_TO_TICKS(2000));
  }
}
