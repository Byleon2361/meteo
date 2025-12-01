#pragma once
#include "hal/adc_types.h"

#define RL_VALUE 200.0f
#define RO_VALUE 146.4f
#define CALIBRATION_SAMPLE_TIMES 50
#define CALIBRATION_SAMPLE_INTERVAL 50

typedef struct {
  float rs_ro_ratio;
  float ppm;
  int raw_adc;
} mq_sensor_data_t;

typedef struct {
  adc_channel_t channel;
} mq_params_data_t;

void mq_sensor_task(void* pvParameter);
