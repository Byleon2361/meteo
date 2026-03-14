#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <stdbool.h>
#include <stdint.h>
#include "pms5003.h"

typedef struct {
    // DHT22
    float temperature_dht;
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

    // BMP280
    float temperature_bmp;
    float pressure;
    uint8_t bmp_valid;

    // PMS5003
    uint16_t pm1_0;
    uint16_t pm2_5;
    uint16_t pm10;
    uint8_t  pms_valid;

    SemaphoreHandle_t mutex;
} sensor_data_t;

void sensor_data_init(void);

// DHT22
void sensor_data_set_dht(float temperature_dht, float humidity, uint8_t valid);
void sensor_data_get_dht(
        float* temperature_dht, float* humidity, uint8_t* valid);

// BMP280
void sensor_data_set_bmp(float temperature_bmp, float pressure, uint8_t valid);
void sensor_data_get_bmp(
        float* temperature_bmp, float* pressure, uint8_t* valid);

// MQ-135
void sensor_data_set_mq(
        int raw_adc,
        float voltage,
        float rs_ro_ratio,
        float co2,
        float lpg,
        float co,
        float nh3);
void sensor_data_get_mq(float* co2, float* lpg, float* co, float* nh3);

// PMS5003
void sensor_data_set_pms5003(const pms5003_data_t *data);
void sensor_data_get_pms5003(uint16_t* pm1_0, uint16_t* pm2_5, uint16_t* pm10, uint8_t* valid);