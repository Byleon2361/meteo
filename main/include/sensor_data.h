#pragma once

#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

typedef struct {
    // DHT22 данные
    float temperature;
    float humidity;
    bool dht_valid;
    
    // MQ-135 данные
    int mq_raw_adc;
    float mq_voltage;
    float mq_rs_ro_ratio;
    float co2_ppm;
    float lpg_ppm;
    float co_ppm;
    float nh3_ppm;
    bool mq_valid;
    
    // Мьютекс для защиты от одновременного доступа
    SemaphoreHandle_t mutex;
} sensor_data_t;

// Инициализация
void sensor_data_init(void);

// Функции для записи данных
void sensor_data_set_dht(float temperature, float humidity);
void sensor_data_set_mq(int raw_adc, float voltage, float rs_ro_ratio,
                        float co2, float lpg, float co, float nh3);

// Функции для чтения данных
void sensor_data_get_dht(float *temperature, float *humidity, bool *valid);
void sensor_data_get_mq(float *co2, float *lpg, float *co, float *nh3, bool *valid);
