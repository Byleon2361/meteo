#include <math.h>
#include "adc.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hal/adc_types.h"
#include "mq135.h"
#include "sensor_data.h"
static const char* TAG = "MQ_SENSOR";
static float s_ro = RO_VALUE;
static float read_voltage_avg(adc_channel_t channel, int samples)
{
float sum = 0;
for (int i = 0; i < samples; i++) {
sum += read_adc_voltage(channel);
vTaskDelay(pdMS_TO_TICKS(20));
}
return sum / samples;
}
static float voltage_to_rs(float vrl)
{
if (vrl <= 0.01f) return INFINITY;
return ((MQ_VCC - vrl) / vrl) * RL_VALUE;
}
static float clamp_ppm(float v, float lo, float hi)
{
if (isnan(v) || isinf(v)) return lo;
if (v < lo) return lo;
if (v > hi) return hi;
return v;
}
static float ppm_from_ratio(float ratio, float a, float b, float lo, float hi)
{
return clamp_ppm(powf(ratio / b, 1.0f / a), lo, hi);
}
static float calculate_co2_ppm(float ratio)
{
float ppm = 400.0f * powf(ratio / RO_CLEAN_AIR_RATIO, -2.769f);
return clamp_ppm(ppm, 0.0f, 5000.0f);
}
void mq135_calibrate(adc_channel_t channel)
{
float vrl = read_voltage_avg(channel, CALIBRATION_SAMPLE_TIMES);
float rs = voltage_to_rs(vrl);
if (isinf(rs) || rs <= 0.0f) {
ESP_LOGW(TAG, "Калибровка не удалась, Ro оставлен по умолчанию");
return;
}
s_ro = rs / RO_CLEAN_AIR_RATIO;
ESP_LOGI(TAG, "Калибровка: Vrl=%.3f Rs=%.1f Ro=%.1f", vrl, rs, s_ro);
}
void mq_sensor_task(void* mq_params)
{
mq_params_data_t* params = (mq_params_data_t*)mq_params;
if (params == NULL) {
ESP_LOGE(TAG, "Параметры датчика не переданы");
vTaskDelete(NULL);
return;
}
ESP_LOGI(TAG, "Задача датчика MQ запущена, канал ADC: %d", params->channel);
while (1) {
int raw_adc = read_adc_raw(params->channel);
float voltage = read_voltage_avg(params->channel, 10);
float rs = voltage_to_rs(voltage);
float ratio = rs / s_ro;
ESP_LOGI(TAG, "Vrl=%.3f Rs=%.1f ratio=%.2f", voltage, rs, ratio);
float co2_ppm = calculate_co2_ppm(ratio);
float co_ppm = ppm_from_ratio(ratio, -0.229115f, 4.98267f, 0.0f, 1000.0f);
float nh3_ppm = ppm_from_ratio(ratio, -0.41162f, 6.6564f, 0.0f, 300.0f);
float lpg_ppm = ppm_from_ratio(ratio, -0.30219f, 3.00802f, 0.0f, 1000.0f);
sensor_data_set_mq(raw_adc, voltage, ratio, co2_ppm, lpg_ppm, co_ppm, nh3_ppm);
vTaskDelay(pdMS_TO_TICKS(params->task_delay_s * 1000));
}
}