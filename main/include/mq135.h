#pragma once
#include "hal/adc_types.h"

#define RL_VALUE 167.0f
#define MQ_VCC 3.3f
#define RO_CLEAN_AIR_RATIO 3.6f
#define RO_VALUE 24.0f
#define CALIBRATION_SAMPLE_TIMES 50
#define CALIBRATION_SAMPLE_INTERVAL 50

typedef struct {
    float rs_ro_ratio;
    float ppm;
    int raw_adc;
} mq_sensor_data_t;

typedef struct {
    adc_channel_t channel;
    uint8_t task_delay_s;
} mq_params_data_t;

void mq135_calibrate(adc_channel_t channel);
void mq_sensor_task(void* pvParameter);