#include "bmp280.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "BMP280";

static esp_err_t bmp280_read_reg(bmp280_dev_t *dev, uint8_t reg_addr,
                                 uint8_t *data, size_t len);
static esp_err_t bmp280_write_reg(bmp280_dev_t *dev, uint8_t reg_addr,
                                  uint8_t data);
static esp_err_t bmp280_read_calibration_data(bmp280_dev_t *dev);
static int32_t bmp280_compensate_temperature(bmp280_dev_t *dev, int32_t adc_T);
static uint32_t bmp280_compensate_pressure(bmp280_dev_t *dev, int32_t adc_P);

static esp_err_t bmp280_read_reg(bmp280_dev_t *dev, uint8_t reg_addr,
                                 uint8_t *data, size_t len)
{
  if (len == 0)
  {
    return ESP_ERR_INVALID_ARG;
  }

  i2c_cmd_handle_t cmd = i2c_cmd_link_create();

  // Старт + запись адреса регистра
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, (dev->i2c_addr << 1) | I2C_MASTER_WRITE, true);
  i2c_master_write_byte(cmd, reg_addr, true);

  if (len > 1)
  {
    // Повторный старт для чтения нескольких байт
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (dev->i2c_addr << 1) | I2C_MASTER_READ, true);

    if (len > 1)
    {
      i2c_master_read(cmd, data, len - 1, I2C_MASTER_ACK);
    }
    i2c_master_read_byte(cmd, data + len - 1, I2C_MASTER_NACK);
  }
  else
  {
    // Чтение одного байта
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (dev->i2c_addr << 1) | I2C_MASTER_READ, true);
    i2c_master_read_byte(cmd, data, I2C_MASTER_NACK);
  }

  i2c_master_stop(cmd);

  esp_err_t ret = i2c_master_cmd_begin(
      dev->i2c_port, cmd, BMP280_I2C_TIMEOUT_MS / portTICK_PERIOD_MS);
  i2c_cmd_link_delete(cmd);

  return ret;
}

/**
 * @brief Запись в регистр
 */
static esp_err_t bmp280_write_reg(bmp280_dev_t *dev, uint8_t reg_addr,
                                  uint8_t data)
{
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();

  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, (dev->i2c_addr << 1) | I2C_MASTER_WRITE, true);
  i2c_master_write_byte(cmd, reg_addr, true);
  i2c_master_write_byte(cmd, data, true);
  i2c_master_stop(cmd);

  esp_err_t ret = i2c_master_cmd_begin(
      dev->i2c_port, cmd, BMP280_I2C_TIMEOUT_MS / portTICK_PERIOD_MS);
  i2c_cmd_link_delete(cmd);

  return ret;
}

/**
 * @brief Чтение калибровочных данных
 */
static esp_err_t bmp280_read_calibration_data(bmp280_dev_t *dev)
{
  esp_err_t err;
  uint8_t buffer[24];

  // Чтение калибровочных данных T1-T3 и P1-P9 (адреса 0x88-0x9F)
  err = bmp280_read_reg(dev, BMP280_REG_DIG_T1, buffer, 24);
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to read calibration data");
    return err;
  }

  // Преобразование данных температуры
  dev->calib_data.dig_T1 = (buffer[1] << 8) | buffer[0];
  dev->calib_data.dig_T2 = (buffer[3] << 8) | buffer[2];
  dev->calib_data.dig_T3 = (buffer[5] << 8) | buffer[4];

  // Преобразование данных давления
  dev->calib_data.dig_P1 = (buffer[7] << 8) | buffer[6];
  dev->calib_data.dig_P2 = (buffer[9] << 8) | buffer[8];
  dev->calib_data.dig_P3 = (buffer[11] << 8) | buffer[10];
  dev->calib_data.dig_P4 = (buffer[13] << 8) | buffer[12];
  dev->calib_data.dig_P5 = (buffer[15] << 8) | buffer[14];
  dev->calib_data.dig_P6 = (buffer[17] << 8) | buffer[16];
  dev->calib_data.dig_P7 = (buffer[19] << 8) | buffer[18];
  dev->calib_data.dig_P8 = (buffer[21] << 8) | buffer[20];
  dev->calib_data.dig_P9 = (buffer[23] << 8) | buffer[22];

  ESP_LOGD(TAG, "Calibration data read successfully");
  return ESP_OK;
}

/**
 * @brief Компенсация температуры (алгоритм из даташита)
 */
static int32_t bmp280_compensate_temperature(bmp280_dev_t *dev, int32_t adc_T)
{
  int32_t var1, var2, T;

  var1 = ((((adc_T >> 3) - ((int32_t)dev->calib_data.dig_T1 << 1))) *
          ((int32_t)dev->calib_data.dig_T2)) >>
         11;

  var2 = (((((adc_T >> 4) - ((int32_t)dev->calib_data.dig_T1)) *
            ((adc_T >> 4) - ((int32_t)dev->calib_data.dig_T1))) >>
           12) *
          ((int32_t)dev->calib_data.dig_T3)) >>
         14;

  dev->t_fine = var1 + var2;

  T = (dev->t_fine * 5 + 128) >> 8;
  return T;
}

/**
 * @brief Компенсация давления (алгоритм из даташита)
 */
static uint32_t bmp280_compensate_pressure(bmp280_dev_t *dev, int32_t adc_P)
{
  int64_t var1, var2, p;

  var1 = ((int64_t)dev->t_fine) - 128000;
  var2 = var1 * var1 * (int64_t)dev->calib_data.dig_P6;
  var2 = var2 + ((var1 * (int64_t)dev->calib_data.dig_P5) << 17);
  var2 = var2 + (((int64_t)dev->calib_data.dig_P4) << 35);
  var1 = ((var1 * var1 * (int64_t)dev->calib_data.dig_P3) >> 8) +
         ((var1 * (int64_t)dev->calib_data.dig_P2) << 12);
  var1 =
      (((((int64_t)1) << 47) + var1)) * ((int64_t)dev->calib_data.dig_P1) >> 33;

  if (var1 == 0)
  {
    return 0;
  }

  p = 1048576 - adc_P;
  p = (((p << 31) - var2) * 3125) / var1;
  var1 = (((int64_t)dev->calib_data.dig_P9) * (p >> 13) * (p >> 13)) >> 25;
  var2 = (((int64_t)dev->calib_data.dig_P8) * p) >> 19;

  p = ((p + var1 + var2) >> 8) + (((int64_t)dev->calib_data.dig_P7) << 4);

  return (uint32_t)p;
}

/**
 * @brief Публичная функция инициализации
 */
esp_err_t bmp280_init(bmp280_dev_t *dev, i2c_port_t i2c_port, uint8_t i2c_addr)
{
  esp_err_t err;
  uint8_t chip_id;

  if (dev == NULL)
  {
    return ESP_ERR_INVALID_ARG;
  }

  memset(dev, 0, sizeof(bmp280_dev_t));
  dev->i2c_port = i2c_port;
  dev->t_fine = 0;

  // Определение адреса устройства
  if (i2c_addr == 0)
  {
    // Автоопределение адреса
    uint8_t addresses[] = {BMP280_I2C_ADDR_PRIMARY, BMP280_I2C_ADDR_SECONDARY};
    bool found = false;

    for (int i = 0; i < 2; i++)
    {
      dev->i2c_addr = addresses[i];

      // Пробуем прочитать ID чипа
      err = bmp280_get_chip_id(dev, &chip_id);
      if (err == ESP_OK)
      {
        if (chip_id == BMP280_CHIP_ID)
        {
          found = true;
          break;
        }
      }
    }

    if (!found)
    {
      ESP_LOGE(TAG, "No BMP280 found on any address");
      return ESP_ERR_NOT_FOUND;
    }
  }
  else
  {
    // Используем указанный адрес
    dev->i2c_addr = i2c_addr;

    // Проверяем наличие устройства
    err = bmp280_get_chip_id(dev, &chip_id);
    if (err != ESP_OK)
    {
      ESP_LOGE(TAG, "No device at address 0x%02X", i2c_addr);
      return ESP_ERR_NOT_FOUND;
    }
  }

  // Проверяем ID чипа
  if (chip_id != BMP280_CHIP_ID)
  {
    ESP_LOGE(TAG, "Invalid chip ID: 0x%02X (expected 0x%02X)", chip_id,
             BMP280_CHIP_ID);
    return ESP_ERR_NOT_SUPPORTED;
  }

  ESP_LOGI(TAG, "Found BMP280 at 0x%02X", dev->i2c_addr);

  // Сброс устройства
  err = bmp280_soft_reset(dev);
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to reset device");
    return err;
  }

  vTaskDelay(pdMS_TO_TICKS(10));

  // Чтение калибровочных данных
  err = bmp280_read_calibration_data(dev);
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to read calibration data");
    return err;
  }

  // Настройки по умолчанию
  dev->config.osrs_t = BMP280_OS_1X;
  dev->config.osrs_p = BMP280_OS_1X;
  dev->config.filter = BMP280_FILTER_OFF;
  dev->config.mode = BMP280_NORMAL_MODE;

  return ESP_OK;
}

/**
 * @brief Настройка параметров датчика
 */
esp_err_t bmp280_set_config(bmp280_dev_t *dev, const bmp280_config_t *config)
{
  esp_err_t err;
  uint8_t ctrl_meas, config_reg;

  if (dev == NULL || config == NULL)
  {
    return ESP_ERR_INVALID_ARG;
  }

  memcpy(&dev->config, config, sizeof(bmp280_config_t));

  // Установка контроля измерений
  ctrl_meas = (config->osrs_t << 5) | (config->osrs_p << 2) | config->mode;
  err = bmp280_write_reg(dev, BMP280_REG_CTRL_MEAS, ctrl_meas);
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to write CTRL_MEAS register");
    return err;
  }

  // Установка конфигурации
  config_reg = (config->filter << 2) | 0x00; // Standby time = 0.5ms
  err = bmp280_write_reg(dev, BMP280_REG_CONFIG, config_reg);
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to write CONFIG register");
    return err;
  }

  return ESP_OK;
}

/**
 * @brief Чтение данных с датчика
 */
esp_err_t bmp280_read_data(bmp280_dev_t *dev, float *temperature,
                           float *pressure)
{
  esp_err_t err;
  uint8_t data[6];
  int32_t adc_T = 0, adc_P = 0;

  if (dev == NULL)
  {
    return ESP_ERR_INVALID_ARG;
  }

  // Чтение данных (6 байт: 3 давления, 3 температуры)
  err = bmp280_read_reg(dev, BMP280_REG_PRESS_MSB, data, 6);
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to read sensor data");
    return err;
  }

  // Извлечение необработанных значений
  adc_P = (int32_t)(((uint32_t)data[0] << 12) | ((uint32_t)data[1] << 4) |
                    ((uint32_t)data[2] >> 4));
  adc_T = (int32_t)(((uint32_t)data[3] << 12) | ((uint32_t)data[4] << 4) |
                    ((uint32_t)data[5] >> 4));

  // Компенсация температуры
  int32_t comp_temp = bmp280_compensate_temperature(dev, adc_T);

  if (temperature != NULL)
  {
    *temperature = comp_temp / 100.0f;
  }

  // Компенсация давления
  if (pressure != NULL)
  {
    uint32_t comp_press = bmp280_compensate_pressure(dev, adc_P);
    *pressure = comp_press / 256.0f; // Результат в Па
  }

  return ESP_OK;
}

/**
 * @brief Программный сброс
 */
esp_err_t bmp280_soft_reset(bmp280_dev_t *dev)
{
  if (dev == NULL)
  {
    return ESP_ERR_INVALID_ARG;
  }

  return bmp280_write_reg(dev, BMP280_REG_RESET, BMP280_RESET_VALUE);
}

/**
 * @brief Получение ID чипа
 */
esp_err_t bmp280_get_chip_id(bmp280_dev_t *dev, uint8_t *chip_id)
{
  if (dev == NULL || chip_id == NULL)
  {
    return ESP_ERR_INVALID_ARG;
  }

  return bmp280_read_reg(dev, BMP280_REG_CHIP_ID, chip_id, 1);
}

/**
 * @brief Установка режима сна
 */
esp_err_t bmp280_sleep(bmp280_dev_t *dev)
{
  if (dev == NULL)
  {
    return ESP_ERR_INVALID_ARG;
  }

  // Читаем текущее значение регистра
  uint8_t ctrl_meas;
  esp_err_t err = bmp280_read_reg(dev, BMP280_REG_CTRL_MEAS, &ctrl_meas, 1);
  if (err != ESP_OK)
  {
    return err;
  }

  // Устанавливаем режим сна
  ctrl_meas &= ~0x03; // Очищаем биты режима
  ctrl_meas |= BMP280_SLEEP_MODE;

  return bmp280_write_reg(dev, BMP280_REG_CTRL_MEAS, ctrl_meas);
}

/**
 * @brief Проверка, выполняется ли измерение
 */
esp_err_t bmp280_is_measuring(bmp280_dev_t *dev, bool *measuring)
{
  if (dev == NULL || measuring == NULL)
  {
    return ESP_ERR_INVALID_ARG;
  }

  uint8_t status;
  esp_err_t err = bmp280_read_reg(dev, BMP280_REG_STATUS, &status, 1);
  if (err != ESP_OK)
  {
    return err;
  }

  *measuring = (status & (1 << 3)) != 0;
  return ESP_OK;
}

/**
 * @brief Проверка, выполняется ли копирование регистров
 */
esp_err_t bmp280_is_updating(bmp280_dev_t *dev, bool *copying)
{
  if (dev == NULL || copying == NULL)
  {
    return ESP_ERR_INVALID_ARG;
  }

  uint8_t status;
  esp_err_t err = bmp280_read_reg(dev, BMP280_REG_STATUS, &status, 1);
  if (err != ESP_OK)
  {
    return err;
  }

  *copying = (status & 1) != 0;
  return ESP_OK;
}
