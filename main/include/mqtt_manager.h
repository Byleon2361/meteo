#pragma once
#include "esp_err.h"
#include <stdbool.h>

esp_err_t mqtt_manager_init(void);
bool      mqtt_manager_is_connected(void);

// Публикация одного float-значения
esp_err_t mqtt_publish_float(const char *topic, const char *key, float value);

// Публикация PM2.5/PM1.0/PM10 одним JSON
esp_err_t mqtt_publish_pm(uint16_t pm1_0, uint16_t pm2_5, uint16_t pm10);

// Публикация газов одним JSON
esp_err_t mqtt_publish_gases(float co2, float co, float nh3, float lpg);

// Публикация всех данных разом (вызывается раз в 5 минут)
esp_err_t mqtt_publish_all(void);

void mqtt_publish_task(void *arg);