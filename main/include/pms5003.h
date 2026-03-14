#pragma once

#include <stdint.h>
#include "driver/uart.h"

// -------------------------------------------------------
//  PMS5003 — датчик частиц (PM1.0 / PM2.5 / PM10)
//  Интерфейс: UART, 9600 бод, 3.3 В логика
// -------------------------------------------------------

#define PMS5003_UART_PORT   UART_NUM_2
#define PMS5003_UART_BAUD   9600
#define PMS5003_BUF_SIZE    64

// Кадр датчика: 0x42 0x4D + 30 байт данных = 32 байта
#define PMS5003_FRAME_LEN   32
#define PMS5003_START1      0x42
#define PMS5003_START2      0x4D

// Данные, которые читаем из датчика
typedef struct {
    uint16_t pm1_0;   // PM1.0  мкг/м³  (атмосферный)
    uint16_t pm2_5;   // PM2.5  мкг/м³  (атмосферный)
    uint16_t pm10;    // PM10   мкг/м³  (атмосферный)
    // Концентрации частиц, шт/0.1 л воздуха
    uint16_t cnt_0_3; // > 0.3 мкм
    uint16_t cnt_0_5; // > 0.5 мкм
    uint16_t cnt_1_0; // > 1.0 мкм
    uint16_t cnt_2_5; // > 2.5 мкм
    uint16_t cnt_5_0; // > 5.0 мкм
    uint16_t cnt_10;  // > 10  мкм
} pms5003_data_t;

// Параметры задачи — передаются через xTaskCreate
typedef struct {
    int      tx_gpio;      // TX пин ESP32 → RX датчика
    int      rx_gpio;      // RX пин ESP32 ← TX датчика
    uint32_t task_delay_s; // интервал обновления, секунды
} pms_params_data_t;

// FreeRTOS-задача опроса датчика
void pms5003_task(void *pvParameters);