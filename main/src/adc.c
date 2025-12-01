#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hal/adc_types.h"
#include "adc.h"

static const char* TAG = "ADC";

static adc_oneshot_unit_handle_t adc1_handle;
static adc_cali_handle_t adc_cali_handle = NULL;

// Чтение сырого значения ADC
int read_adc_raw(adc_channel_t channel) {
  int adc_reading = 0;

  // Усреднение нескольких измерений
  for (int i = 0; i < 10; i++) {
    int raw;
    ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, channel, &raw));
    adc_reading += raw;
    vTaskDelay(pdMS_TO_TICKS(10));
  }

  return adc_reading / 10;
}

// Конвертация ADC в напряжение
float read_adc_voltage(adc_channel_t channel) {
  int adc_raw = read_adc_raw(channel);

  if (adc_cali_handle) {
    int voltage;
    esp_err_t ret = adc_cali_raw_to_voltage(adc_cali_handle, adc_raw, &voltage);
    if (ret == ESP_OK) {
      return (float)voltage / 1000.0;  // В вольтах
    }
  }

  // Если калибровка недоступна, используем приблизительный расчет
  return (float)adc_raw * 3.3f / 4095.0f;
}

// Инициализация калибровки ADC
static bool adc_calibration_init(adc_unit_t unit, adc_channel_t channel,
                                 adc_atten_t atten,
                                 adc_cali_handle_t* out_handle) {
  adc_cali_handle_t handle = NULL;
  esp_err_t ret = ESP_FAIL;
  bool calibrated = false;

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
  if (!calibrated) {
    ESP_LOGI(TAG, "Калибровка ADC с использованием Curve Fitting");
    adc_cali_curve_fitting_config_t cali_config = {
        .unit_id = unit,
        .chan = channel,
        .atten = atten,
        .bitwidth = ADC_BIT_WIDTH,
    };
    ret = adc_cali_create_scheme_curve_fitting(&cali_config, &handle);
    if (ret == ESP_OK) {
      calibrated = true;
    }
  }
#endif

#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
  if (!calibrated) {
    ESP_LOGI(TAG, "Калибровка ADC с использованием Linear Fitting");
    adc_cali_line_fitting_config_t cali_config = {
        .unit_id = unit,
        .atten = atten,
        .bitwidth = ADC_BIT_WIDTH,
    };
    ret = adc_cali_create_scheme_line_fitting(&cali_config, &handle);
    if (ret == ESP_OK) {
      calibrated = true;
    }
  }
#endif

  *out_handle = handle;
  if (ret == ESP_OK) {
    ESP_LOGI(TAG, "Калибровка ADC успешна");
  } else if (ret == ESP_ERR_NOT_SUPPORTED || !calibrated) {
    ESP_LOGW(TAG,
             "Калибровка не поддерживается, используем сырые значения ADC");
  } else {
    ESP_LOGE(TAG, "Ошибка калибровки ADC");
  }

  return calibrated;
}
// Инициализация ADC
void adc_init(adc_channel_t channel) {
  // Конфигурация ADC
  adc_oneshot_unit_init_cfg_t init_config1 = {
      .unit_id = ADC_UNIT,
      .ulp_mode = ADC_ULP_MODE_DISABLE,
  };
  ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

  // Конфигурация канала
  adc_oneshot_chan_cfg_t config = {
      .atten = ADC_ATTEN,
      .bitwidth = ADC_BIT_WIDTH,
  };
  ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, channel, &config));

  // Инициализация калибровки
  adc_calibration_init(ADC_UNIT, channel, ADC_ATTEN, &adc_cali_handle);
}