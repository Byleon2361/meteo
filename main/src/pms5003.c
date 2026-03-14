#include "pms5003.h"
#include "sensor_data.h"

#include "driver/uart.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "PMS5003";

// -------------------------------------------------------
//  Инициализация UART для PMS5003
// -------------------------------------------------------
static esp_err_t pms5003_uart_init(int tx_gpio, int rx_gpio)
{
    uart_config_t cfg = {
        .baud_rate  = PMS5003_UART_BAUD,
        .data_bits  = UART_DATA_8_BITS,
        .parity     = UART_PARITY_DISABLE,
        .stop_bits  = UART_STOP_BITS_1,
        .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    esp_err_t err;

    err = uart_param_config(PMS5003_UART_PORT, &cfg);
    if (err != ESP_OK) return err;

    err = uart_set_pin(PMS5003_UART_PORT,
                       tx_gpio, rx_gpio,
                       UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (err != ESP_OK) return err;

    err = uart_driver_install(PMS5003_UART_PORT,
                              PMS5003_BUF_SIZE * 4,  // RX буфер
                              0,                      // TX буфер (0 = синхронный)
                              0, NULL, 0);
    return err;
}

// -------------------------------------------------------
//  Разбор 32-байтного кадра датчика
//  Возвращает ESP_OK если кадр корректен
// -------------------------------------------------------
static esp_err_t pms5003_parse(const uint8_t *buf, pms5003_data_t *out)
{
    // Стартовые байты
    if (buf[0] != PMS5003_START1 || buf[1] != PMS5003_START2) {
        ESP_LOGW(TAG, "Неверные стартовые байты: 0x%02X 0x%02X", buf[0], buf[1]);
        return ESP_ERR_INVALID_RESPONSE;
    }

    // Длина фрейма (байты 2-3) должна быть 0x001C = 28
    uint16_t frame_len = (buf[2] << 8) | buf[3];
    if (frame_len != 0x001C) {
        ESP_LOGW(TAG, "Неверная длина кадра: %u", frame_len);
        return ESP_ERR_INVALID_SIZE;
    }

    // Контрольная сумма — сумма всех байт кроме двух последних
    uint16_t checksum = 0;
    for (int i = 0; i < PMS5003_FRAME_LEN - 2; i++) {
        checksum += buf[i];
    }
    uint16_t recv_csum = (buf[30] << 8) | buf[31];
    if (checksum != recv_csum) {
        ESP_LOGW(TAG, "Ошибка контрольной суммы: вычислено 0x%04X, получено 0x%04X",
                 checksum, recv_csum);
        return ESP_ERR_INVALID_CRC;
    }

    // Атмосферные концентрации (байты 10..15)
    out->pm1_0 = (buf[10] << 8) | buf[11];
    out->pm2_5 = (buf[12] << 8) | buf[13];
    out->pm10  = (buf[14] << 8) | buf[15];

    // Счётчики частиц (байты 16..27)
    out->cnt_0_3 = (buf[16] << 8) | buf[17];
    out->cnt_0_5 = (buf[18] << 8) | buf[19];
    out->cnt_1_0 = (buf[20] << 8) | buf[21];
    out->cnt_2_5 = (buf[22] << 8) | buf[23];
    out->cnt_5_0 = (buf[24] << 8) | buf[25];
    out->cnt_10  = (buf[26] << 8) | buf[27];

    return ESP_OK;
}

// -------------------------------------------------------
//  Чтение одного кадра из UART
//  Ищем стартовые байты 0x42 0x4D, затем читаем остаток
// -------------------------------------------------------
static esp_err_t pms5003_read_frame(uint8_t *frame_buf)
{
    uint8_t byte;

    // Ищем первый стартовый байт 0x42
    while (1) {
        int n = uart_read_bytes(PMS5003_UART_PORT, &byte, 1, pdMS_TO_TICKS(2000));
        if (n <= 0) {
            ESP_LOGW(TAG, "Таймаут ожидания данных от датчика");
            return ESP_ERR_TIMEOUT;
        }
        if (byte == PMS5003_START1) break;
    }

    // Ищем второй стартовый байт 0x4D
    int n = uart_read_bytes(PMS5003_UART_PORT, &byte, 1, pdMS_TO_TICKS(100));
    if (n <= 0 || byte != PMS5003_START2) {
        return ESP_ERR_INVALID_RESPONSE;
    }

    // Собираем полный кадр
    frame_buf[0] = PMS5003_START1;
    frame_buf[1] = PMS5003_START2;

    int remaining = PMS5003_FRAME_LEN - 2;
    int received  = uart_read_bytes(PMS5003_UART_PORT,
                                    frame_buf + 2,
                                    remaining,
                                    pdMS_TO_TICKS(500));
    if (received != remaining) {
        ESP_LOGW(TAG, "Неполный кадр: получено %d из %d байт", received, remaining);
        return ESP_ERR_INVALID_SIZE;
    }

    return ESP_OK;
}

// -------------------------------------------------------
//  FreeRTOS задача
// -------------------------------------------------------
void pms5003_task(void *pvParameters)
{
    pms_params_data_t *params = (pms_params_data_t *)pvParameters;

    ESP_LOGI(TAG, "Инициализация UART (TX=%d, RX=%d, %d бод)...",
             params->tx_gpio, params->rx_gpio, PMS5003_UART_BAUD);

    if (pms5003_uart_init(params->tx_gpio, params->rx_gpio) != ESP_OK) {
        ESP_LOGE(TAG, "Не удалось инициализировать UART. Задача завершена.");
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "PMS5003 запущен, интервал %lu с", (unsigned long)params->task_delay_s);

    // Датчику нужно ~30 с после включения для стабилизации
    vTaskDelay(pdMS_TO_TICKS(30000));

    uint8_t        frame[PMS5003_FRAME_LEN];
    pms5003_data_t data;

    while (1) {
        esp_err_t err = pms5003_read_frame(frame);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "Ошибка чтения кадра: %s", esp_err_to_name(err));
            vTaskDelay(pdMS_TO_TICKS(params->task_delay_s * 1000));
            continue;
        }

        err = pms5003_parse(frame, &data);
        if (err != ESP_OK) {
            vTaskDelay(pdMS_TO_TICKS(params->task_delay_s * 1000));
            continue;
        }

        ESP_LOGI(TAG, "PM1.0=%u  PM2.5=%u  PM10=%u  мкг/м³",
                 data.pm1_0, data.pm2_5, data.pm10);

        sensor_data_set_pms5003(&data);

        vTaskDelay(pdMS_TO_TICKS(params->task_delay_s * 1000));
    }
}