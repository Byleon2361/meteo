#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

typedef struct {
    // DHT22 
    float temperature;
    float humidity;
    uint8_t dht_valid;
    
    // MQ-135 
    int mq_raw_adc;
    float mq_voltage;
    float mq_rs_ro_ratio;
    float co2_ppm;
    float lpg_ppm;
    float co_ppm;
    float nh3_ppm;
    
    SemaphoreHandle_t mutex;
} sensor_data_t;

void sensor_data_init(void);

void sensor_data_set_dht(float temperature, float humidity, uint8_t valid);
void sensor_data_set_mq(int raw_adc, float voltage, float rs_ro_ratio, float co2, float lpg, float co, float nh3);

void sensor_data_get_dht(float *temperature, float *humidity, uint8_t *valid);
void sensor_data_get_mq(float *co2, float *lpg, float *co, float *nh3);
