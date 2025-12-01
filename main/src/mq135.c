#include <math.h>

#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hal/adc_types.h"
#include "adc.h"
#include "mq135.h"
#include "sensor_data.h"

static const char* TAG = "MQ_SENSOR";
#include "webserver.h"

// #include <stdlib.h>
// #include <string.h>

// #include "color_storage.h"
// #include "esp_err.h"
// #include "esp_http_server.h"
// #include "esp_log.h"
// #include "rgb_led.h"
// #include "sensor_data.h"

// static const char* TAG = "WEB";

// extern g_sensor_data data;

// // EMBED_FILES: data/page.html, data/style.css, data/script.js
// extern const uint8_t _binary_page_html_gz_start[];
// extern const uint8_t _binary_page_html_gz_end[];

// extern const uint8_t _binary_style_css_gz_start[];
// extern const uint8_t _binary_style_css_gz_end[];

// extern const uint8_t _binary_script_js_gz_start[];
// extern const uint8_t _binary_script_js_gz_end[];

// static esp_err_t root_handler(httpd_req_t* req) {
//   const uint8_t* data = _binary_page_html_gz_start;
//   size_t size = _binary_page_html_gz_end - _binary_page_html_gz_start;
//   httpd_resp_set_type(req, "text/html; charset=utf-8");
//   httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
//   httpd_resp_set_hdr(req, "Cache-Control",
//                      "no-cache, no-store, must-revalidate");
//   httpd_resp_set_hdr(req, "Pragma", "no-cache");
//   httpd_resp_set_hdr(req, "Expires", "0");
//   httpd_resp_send(req, (const char*)data, size);
//   return ESP_OK;
// }

// static esp_err_t css_handler(httpd_req_t* req) {
//   const uint8_t* data = _binary_style_css_gz_start;
//   size_t size = _binary_style_css_gz_end - _binary_style_css_gz_start;
//   httpd_resp_set_type(req, "text/css; charset=utf-8");
//   httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
//   httpd_resp_send(req, (const char*)data, size);
//   return ESP_OK;
// }

// static esp_err_t js_handler(httpd_req_t* req) {
//   const uint8_t* data = _binary_script_js_gz_start;
//   size_t size = _binary_script_js_gz_end - _binary_script_js_gz_start;
//   httpd_resp_set_type(req, "application/javascript; charset=utf-8");
//   httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
//   httpd_resp_send(req, (const char*)data, size);
//   return ESP_OK;
// }

// static esp_err_t get_handler(httpd_req_t* req) {
//   uint8_t r = 0, g = 0, b = 0;
//   esp_err_t err = color_storage_load(&r, &g, &b);
//   if (err != ESP_OK) {
//     r = g = b = 0;
//   }
//   char buf[64];
//   int len = snprintf(buf, sizeof(buf), "{\"temperature\":%u,\"humidity\":%u,\"CO2\":%u, \"CO\":%u, \"NH3\":%u, \"LPG\":%u,}", r, g, b);
//   httpd_resp_set_type(req, "application/json; charset=utf-8");
//   httpd_resp_send(req, buf, len);
//   return ESP_OK;
// }

// static esp_err_t set_handler(httpd_req_t* req) {
//   char query[64];
//   char param[8];
//   int r = 0, g = 0, b = 0;
//   if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK) {
//     if (httpd_query_key_value(query, "r", param, sizeof(param)) == ESP_OK)
//       r = atoi(param);
//     if (httpd_query_key_value(query, "g", param, sizeof(param)) == ESP_OK)
//       g = atoi(param);
//     if (httpd_query_key_value(query, "b", param, sizeof(param)) == ESP_OK)
//       b = atoi(param);
//   }
//   if (r < 0) r = 0;
//   if (r > 255) r = 255;
//   if (g < 0) g = 0;
//   if (g > 255) g = 255;
//   if (b < 0) b = 0;
//   if (b > 255) b = 255;
//   rgb_led_set((uint8_t)r, (uint8_t)g, (uint8_t)b);
//   color_storage_save((uint8_t)r, (uint8_t)g, (uint8_t)b);
//   char resp[64];
//   snprintf(resp, sizeof(resp), "OK R=%d G=%d B=%d", r, g, b);
//   httpd_resp_set_type(req, "text/plain; charset=utf-8");
//   httpd_resp_send(req, resp, strlen(resp));
//   return ESP_OK;
// }
// void start_webserver(void) {
//   httpd_config_t config = HTTPD_DEFAULT_CONFIG();
//   httpd_handle_t server = NULL;
//   if (httpd_start(&server, &config) != ESP_OK) {
//     ESP_LOGE(TAG, "Failed to start HTTP server");
//     return;
//   }
//   httpd_uri_t root = {.uri = "/",
//                       .method = HTTP_GET,
//                       .handler = root_handler,
//                       .user_ctx = NULL};
//   httpd_register_uri_handler(server, &root);
//   httpd_uri_t css = {.uri = "/style.css",
//                      .method = HTTP_GET,
//                      .handler = css_handler,
//                      .user_ctx = NULL};
//   httpd_register_uri_handler(server, &css);
//   httpd_uri_t js = {.uri = "/script.js",
//                     .method = HTTP_GET,
//                     .handler = js_handler,
//                     .user_ctx = NULL};
//   httpd_register_uri_handler(server, &js);
//   httpd_uri_t set = {.uri = "/set",
//                      .method = HTTP_GET,
//                      .handler = set_handler,
//                      .user_ctx = NULL};
//   httpd_register_uri_handler(server, &set);
//   httpd_uri_t get = {.uri = "/get",
//                      .method = HTTP_GET,
//                      .handler = get_handler,
//                      .user_ctx = NULL};
//   httpd_register_uri_handler(server, &get);
//   ESP_LOGI(TAG, "HTTP server started");
// }
// Расчет сопротивления датчика
static float calculate_rs(float vrl) {
  float vcc = 3.3;  // Напряжение питания
  return ((vcc - vrl) / vrl) * RL_VALUE;
}

// Расчет отношения Rs/Ro
static float calculate_rs_ro_ratio(adc_channel_t channel, float ro) {
  float voltage = read_adc_voltage(channel);
  float rs = calculate_rs(voltage);
  return rs / ro;
}

// Пример расчета PPM для MQ-135 (CO2)
static float calculate_co2_ppm(float rs_ro_ratio) {
  // Коэффициенты для кривой MQ-135 (CO2)
  float a = 116.6020682;
  float b = -2.769034857;

  return a * powf(rs_ro_ratio, b);
}

// Пример расчета PPM для MQ-2 (LPG)
static float calculate_lpg_ppm(float rs_ro_ratio) {
  // Коэффициенты для кривой MQ-2 (LPG)
  float a = 574.25;
  float b = -2.222;

  return a * powf(rs_ro_ratio, b);
}

// Пример расчета PPM для MQ-7 (CO)
static float calculate_co_ppm(float rs_ro_ratio) {
  // Коэффициенты для кривой MQ-7 (CO)
  float a = 99.042;
  float b = -1.518;

  return a * powf(rs_ro_ratio, b);
}

// Пример расчета PPM для MQ-135 (NH3/аммиак)
static float calculate_nh3_ppm(float rs_ro_ratio) {
  // Коэффициенты для кривой MQ-135 (NH3)
  float a = 102.2;
  float b = -2.473;

  return a * powf(rs_ro_ratio, b);
}

void mq_sensor_task(void* pvParameter) 
{
    mq_params_data_t *params = (mq_params_data_t*)pvParameter;
    
    if (params == NULL) {
        ESP_LOGE(TAG, "Параметры датчика не переданы!");
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "Задача датчика MQ запущена");
    ESP_LOGI(TAG, "Канал ADC: %d", params->channel);

    while (1) {
        // Чтение данных
        int raw_adc = read_adc_raw(params->channel);
        float voltage = read_adc_voltage(params->channel);
        float rs_ro_ratio = calculate_rs_ro_ratio(params->channel, RO_VALUE);
        float co2_ppm = calculate_co2_ppm(rs_ro_ratio);
        float lpg_ppm = calculate_lpg_ppm(rs_ro_ratio);
        float co_ppm = calculate_co_ppm(rs_ro_ratio);
        float nh3_ppm = calculate_nh3_ppm(rs_ro_ratio);

        // Запись в общую структуру
        sensor_data_set_mq(raw_adc, voltage, rs_ro_ratio, 
                          co2_ppm, lpg_ppm, co_ppm, nh3_ppm);

        // Логирование (опционально)
        ESP_LOGI(TAG, "MQ-135: ADC=%d, %.3fV, Rs/Ro=%.2f, CO2=%.2fppm", 
                raw_adc, voltage, rs_ro_ratio, co2_ppm);

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}