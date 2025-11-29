#include <math.h>
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

static const char* TAG = "MQ_SENSOR";

// Конфигурация ADC
#define ADC_CHANNEL ADC_CHANNEL_0  // GPIO36
#define ADC_UNIT ADC_UNIT_1
#define ADC_BIT_WIDTH ADC_BITWIDTH_12
#define ADC_ATTEN ADC_ATTEN_DB_12

// Калибровочные константы (нужно настроить под конкретный датчик)
#define RL_VALUE 5.0       // Сопротивление нагрузки в кОм
#define RO_CLEAN_AIR 9.83  // Сопротивление в чистом воздухе
#define CALIBRATION_SAMPLE_TIMES 50
#define CALIBRATION_SAMPLE_INTERVAL 50

// Структура для хранения данных датчика
typedef struct {
  float rs_ro_ratio;
  float ppm;
  int raw_adc;
} mq_sensor_data_t;

static adc_oneshot_unit_handle_t adc1_handle;
static adc_cali_handle_t adc_cali_handle = NULL;

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
void adc_init(void) {
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
  ESP_ERROR_CHECK(
      adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL, &config));

  // Инициализация калибровки
  adc_calibration_init(ADC_UNIT, ADC_CHANNEL, ADC_ATTEN, &adc_cali_handle);
}

// Чтение сырого значения ADC
int read_adc_raw(void) {
  int adc_reading = 0;

  // Усреднение нескольких измерений
  for (int i = 0; i < 10; i++) {
    int raw;
    ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, ADC_CHANNEL, &raw));
    adc_reading += raw;
    vTaskDelay(pdMS_TO_TICKS(10));
  }

  return adc_reading / 10;
}

// Конвертация ADC в напряжение
float read_adc_voltage(void) {
  int adc_raw = read_adc_raw();

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

// Расчет сопротивления датчика
float calculate_rs(float vrl) {
  float vcc = 3.3;  // Напряжение питания
  return ((vcc - vrl) / vrl) * RL_VALUE;
}

// Калибровка датчика в чистом воздухе
float calibrate_sensor(void) {
  ESP_LOGI(TAG, "Калибровка датчика...");

  float rs_sum = 0;
  for (int i = 0; i < CALIBRATION_SAMPLE_TIMES; i++) {
    float voltage = read_adc_voltage();
    float rs = calculate_rs(voltage);
    rs_sum += rs;
    vTaskDelay(pdMS_TO_TICKS(CALIBRATION_SAMPLE_INTERVAL));
    if (i % 10 == 0) {
      ESP_LOGI(TAG, "Калибровка: %d/%d, Напряжение: %.3fV", i,
               CALIBRATION_SAMPLE_TIMES, voltage);
    }
  }

  float rs_avg = rs_sum / CALIBRATION_SAMPLE_TIMES;
  float ro = rs_avg / RO_CLEAN_AIR;

  ESP_LOGI(TAG, "Калибровка завершена. Ro = %.2f кОм", ro);
  return ro;
}

// Расчет отношения Rs/Ro
float calculate_rs_ro_ratio(float ro) {
  float voltage = read_adc_voltage();
  float rs = calculate_rs(voltage);
  return rs / ro;
}

// Пример расчета PPM для MQ-135 (CO2)
float calculate_co2_ppm(float rs_ro_ratio) {
  // Коэффициенты для кривой MQ-135 (CO2)
  float a = 116.6020682;
  float b = -2.769034857;

  return a * powf(rs_ro_ratio, b);
}

// Пример расчета PPM для MQ-2 (LPG)
float calculate_lpg_ppm(float rs_ro_ratio) {
  // Коэффициенты для кривой MQ-2 (LPG)
  float a = 574.25;
  float b = -2.222;

  return a * powf(rs_ro_ratio, b);
}

// Пример расчета PPM для MQ-7 (CO)
float calculate_co_ppm(float rs_ro_ratio) {
  // Коэффициенты для кривой MQ-7 (CO)
  float a = 99.042;
  float b = -1.518;

  return a * powf(rs_ro_ratio, b);
}

// Пример расчета PPM для MQ-135 (NH3/аммиак)
float calculate_nh3_ppm(float rs_ro_ratio) {
  // Коэффициенты для кривой MQ-135 (NH3)
  float a = 102.2;
  float b = -2.473;

  return a * powf(rs_ro_ratio, b);
}

// Задача для чтения данных с датчика
void mq_sensor_task(void* pvParameter) {
  ESP_LOGI(TAG, "Задача датчика MQ запущена");

  // Даем время на прогрев датчика
  ESP_LOGI(TAG, "Прогрев датчика... 10 секунд");
  for (int i = 10; i > 0; i--) {
    ESP_LOGI(TAG, "Осталось %d секунд...", i);
    vTaskDelay(pdMS_TO_TICKS(1000));
  }

  // Калибровка при старте
  float ro = calibrate_sensor();

  ESP_LOGI(TAG, "Начинаем чтение данных с датчика...");

  while (1) {
    mq_sensor_data_t sensor_data;

    // Чтение сырых данных
    sensor_data.raw_adc = read_adc_raw();
    float voltage = read_adc_voltage();
    sensor_data.rs_ro_ratio = calculate_rs_ro_ratio(ro);

    // Расчет PPM для разных газов
    sensor_data.ppm = calculate_co2_ppm(sensor_data.rs_ro_ratio);
    float lpg_ppm = calculate_lpg_ppm(sensor_data.rs_ro_ratio);
    float co_ppm = calculate_co_ppm(sensor_data.rs_ro_ratio);
    float nh3_ppm = calculate_nh3_ppm(sensor_data.rs_ro_ratio);

    // Логирование данных
    ESP_LOGI(TAG, "ADC: %d, Напряжение: %.3fV, Rs/Ro: %.2f",
             sensor_data.raw_adc, voltage, sensor_data.rs_ro_ratio);
    ESP_LOGI(TAG, "CO2: %.2f ppm, LPG: %.2f ppm", sensor_data.ppm, lpg_ppm);
    ESP_LOGI(TAG, "CO: %.2f ppm, NH3: %.2f ppm", co_ppm, nh3_ppm);
    ESP_LOGI(TAG, "----------------------------------------");

    vTaskDelay(pdMS_TO_TICKS(2000));  // Чтение каждые 2 секунды
  }
}

void app_main(void) {
  ESP_LOGI(TAG, "Инициализация датчика MQ...");

  // Инициализация ADC
  adc_init();

  // Создание задачи для датчика
  xTaskCreate(mq_sensor_task, "mq_sensor_task", 4096, NULL, 5, NULL);

  ESP_LOGI(TAG, "Датчик MQ инициализирован, задача создана");

  // Главная задача просто ждет - не выводит лишних сообщений
  while (1) {
    vTaskDelay(portMAX_DELAY);  // Бесконечное ожидание
  }
}