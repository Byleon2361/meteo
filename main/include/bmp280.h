#ifndef BMP_H
#define BMP_H

#include "driver/i2c.h"
#include "esp_log.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// Адреса I2C
#define BMP280_I2C_ADDR_PRIMARY     0x76
#define BMP280_I2C_ADDR_SECONDARY   0x77
#define BMP280_I2C_TIMEOUT_MS       1000

// ID чипов
#define BMP280_CHIP_ID              0x58
#define BMP280_CHIP_ID_VAL          0x58

// Регистры
#define BMP280_REG_CHIP_ID          0xD0
#define BMP280_REG_RESET            0xE0
#define BMP280_REG_STATUS           0xF3
#define BMP280_REG_CTRL_MEAS        0xF4
#define BMP280_REG_CONFIG           0xF5
#define BMP280_REG_PRESS_MSB        0xF7
#define BMP280_REG_PRESS_LSB        0xF8
#define BMP280_REG_PRESS_XLSB       0xF9
#define BMP280_REG_TEMP_MSB         0xFA
#define BMP280_REG_TEMP_LSB         0xFB
#define BMP280_REG_TEMP_XLSB        0xFC

// Калибровочные регистры
#define BMP280_REG_DIG_T1           0x88
#define BMP280_REG_DIG_T2           0x8A
#define BMP280_REG_DIG_T3           0x8C
#define BMP280_REG_DIG_P1           0x8E
#define BMP280_REG_DIG_P2           0x90
#define BMP280_REG_DIG_P3           0x92
#define BMP280_REG_DIG_P4           0x94
#define BMP280_REG_DIG_P5           0x96
#define BMP280_REG_DIG_P6           0x98
#define BMP280_REG_DIG_P7           0x9A
#define BMP280_REG_DIG_P8           0x9C
#define BMP280_REG_DIG_P9           0x9E

// Значения
#define BMP280_RESET_VALUE          0xB6

// Режимы работы
typedef enum {
    BMP280_SLEEP_MODE   = 0,
    BMP280_FORCED_MODE  = 1,
    BMP280_NORMAL_MODE  = 3
} bmp280_mode_t;

// Настройки oversampling
typedef enum {
    BMP280_OS_SKIPPED   = 0,
    BMP280_OS_1X        = 1,
    BMP280_OS_2X        = 2,
    BMP280_OS_4X        = 3,
    BMP280_OS_8X        = 4,
    BMP280_OS_16X       = 5
} bmp280_oversampling_t;

// Настройки фильтра
typedef enum {
    BMP280_FILTER_OFF   = 0,
    BMP280_FILTER_2     = 1,
    BMP280_FILTER_4     = 2,
    BMP280_FILTER_8     = 3,
    BMP280_FILTER_16    = 4
} bmp280_filter_t;

// Структура калибровочных данных
typedef struct {
    uint16_t dig_T1;
    int16_t  dig_T2;
    int16_t  dig_T3;
    uint16_t dig_P1;
    int16_t  dig_P2;
    int16_t  dig_P3;
    int16_t  dig_P4;
    int16_t  dig_P5;
    int16_t  dig_P6;
    int16_t  dig_P7;
    int16_t  dig_P8;
    int16_t  dig_P9;
} bmp280_calib_data_t;

// Структура настроек
typedef struct {
    bmp280_oversampling_t osrs_t;      // Oversampling температуры
    bmp280_oversampling_t osrs_p;      // Oversampling давления
    bmp280_filter_t filter;           // Коэффициент фильтра
    bmp280_mode_t mode;               // Режим работы
} bmp280_config_t;

// Основная структура устройства
typedef struct {
    i2c_port_t i2c_port;
    uint8_t i2c_addr;
    bmp280_calib_data_t calib_data;
    bmp280_config_t config;
    int32_t t_fine;
} bmp280_dev_t;

/**
 * @brief Инициализация BMP280
 * 
 * @param dev Указатель на структуру устройства
 * @param i2c_port Номер порта I2C
 * @param i2c_addr Адрес I2C (0x76 или 0x77), если 0 - автоопределение
 * @return esp_err_t 
 */
esp_err_t bmp280_init(bmp280_dev_t *dev, i2c_port_t i2c_port, uint8_t i2c_addr);

/**
 * @brief Настройка параметров датчика
 * 
 * @param dev Указатель на структуру устройства
 * @param config Указатель на структуру настроек
 * @return esp_err_t 
 */
esp_err_t bmp280_set_config(bmp280_dev_t *dev, const bmp280_config_t *config);

/**
 * @brief Чтение данных с датчика
 * 
 * @param dev Указатель на структуру устройства
 * @param temperature Указатель для температуры (°C)
 * @param pressure Указатель для давления (Па)
 * @return esp_err_t 
 */
esp_err_t bmp280_read_data(bmp280_dev_t *dev, float *temperature, float *pressure);

/**
 * @brief Выполнение программного сброса
 * 
 * @param dev Указатель на структуру устройства
 * @return esp_err_t 
 */
esp_err_t bmp280_soft_reset(bmp280_dev_t *dev);

/**
 * @brief Получение ID чипа
 * 
 * @param dev Указатель на структуру устройства
 * @param chip_id Указатель для ID чипа
 * @return esp_err_t 
 */
esp_err_t bmp280_get_chip_id(bmp280_dev_t *dev, uint8_t *chip_id);

/**
 * @brief Установка режима сна
 * 
 * @param dev Указатель на структуру устройства
 * @return esp_err_t 
 */
esp_err_t bmp280_sleep(bmp280_dev_t *dev);

/**
 * @brief Проверка, выполняется ли измерение
 * 
 * @param dev Указатель на структуру устройства
 * @param measuring Указатель для результата
 * @return esp_err_t 
 */
esp_err_t bmp280_is_measuring(bmp280_dev_t *dev, bool *measuring);

/**
 * @brief Проверка, выполняется ли копирование регистров
 * 
 * @param dev Указатель на структуру устройства
 * @param copying Указатель для результата
 * @return esp_err_t 
 */
esp_err_t bmp280_is_updating(bmp280_dev_t *dev, bool *copying);

#ifdef __cplusplus
}
#endif

#endif // BMP_H