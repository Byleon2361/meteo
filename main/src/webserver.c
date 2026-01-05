#include "webserver.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "esp_err.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "sensor_data.h"
#include "relay.h"

static const char* TAG = "WEB";

// EMBED_FILES: data/page.html, data/style.css, data/script.js
extern const uint8_t _binary_page_html_gz_start[];
extern const uint8_t _binary_page_html_gz_end[];

extern const uint8_t _binary_style_css_gz_start[];
extern const uint8_t _binary_style_css_gz_end[];

extern const uint8_t _binary_script_js_gz_start[];
extern const uint8_t _binary_script_js_gz_end[];

static esp_err_t root_handler(httpd_req_t* req) {
  const uint8_t* data = _binary_page_html_gz_start;
  size_t size = _binary_page_html_gz_end - _binary_page_html_gz_start;
  httpd_resp_set_type(req, "text/html; charset=utf-8");
  httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
  httpd_resp_set_hdr(req, "Cache-Control",
                     "no-cache, no-store, must-revalidate");
  httpd_resp_set_hdr(req, "Pragma", "no-cache");
  httpd_resp_set_hdr(req, "Expires", "0");
  httpd_resp_send(req, (const char*)data, size);
  return ESP_OK;
}

static esp_err_t css_handler(httpd_req_t* req) {
  const uint8_t* data = _binary_style_css_gz_start;
  size_t size = _binary_style_css_gz_end - _binary_style_css_gz_start;
  httpd_resp_set_type(req, "text/css; charset=utf-8");
  httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
  httpd_resp_send(req, (const char*)data, size);
  return ESP_OK;
}

static esp_err_t js_handler(httpd_req_t* req) {
  const uint8_t* data = _binary_script_js_gz_start;
  size_t size = _binary_script_js_gz_end - _binary_script_js_gz_start;
  httpd_resp_set_type(req, "application/javascript; charset=utf-8");
  httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
  httpd_resp_send(req, (const char*)data, size);
  return ESP_OK;
}
static esp_err_t get_handler(httpd_req_t* req) {
  float temperature, humidity;
  uint8_t dht_valid;
  sensor_data_get_dht(&temperature, &humidity, &dht_valid);

  float co2_ppm, lpg_ppm, co_ppm, nh3_ppm;
  sensor_data_get_mq(&co2_ppm, &lpg_ppm, &co_ppm, &nh3_ppm);

  char buf[256];
  int len = snprintf(buf, sizeof(buf), 
      "{\"temperature\":%.2f,\"humidity\":%.2f,\"CO2\":%.2f,\"CO\":%.2f,\"NH3\":%.2f,\"LPG\":%.2f,\"dht_valid\":%d}",
      temperature, humidity, co2_ppm, co_ppm, nh3_ppm, lpg_ppm,
      dht_valid ? 1 : 0
  );
  
  ESP_LOGI(TAG, "Отправляем JSON: %s", buf);
  
  httpd_resp_set_type(req, "application/json");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  httpd_resp_send(req, buf, len);
  return ESP_OK;
}

static esp_err_t relay_handler(httpd_req_t* req) {
    char query[64];
    char state[8];
    
    // Получаем параметр state из URL
    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK) {
        if (httpd_query_key_value(query, "state", state, sizeof(state)) == ESP_OK) {
            
            if (strcmp(state, "on") == 0) {
                relay_on();
                ESP_LOGI(TAG, "Реле ВКЛЮЧЕНО");
                httpd_resp_send(req, "Реле ВКЛЮЧЕНО", HTTPD_RESP_USE_STRLEN);
            } 
            else if (strcmp(state, "off") == 0) {
                relay_off();
                ESP_LOGI(TAG, "Реле ВЫКЛЮЧЕНО");
                httpd_resp_send(req, "Реле ВЫКЛЮЧЕНО", HTTPD_RESP_USE_STRLEN);
            }
            else {
                httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Неверный параметр state");
                return ESP_FAIL;
            }
        } else {
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Параметр state не найден");
            return ESP_FAIL;
        }
    } else {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Ошибка парсинга query");
        return ESP_FAIL;
    }
    
    return ESP_OK;
}
void start_webserver(void) {
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  httpd_handle_t server = NULL;
  if (httpd_start(&server, &config) != ESP_OK) {
    ESP_LOGE(TAG, "Failed to start HTTP server");
    return;
  }
  httpd_uri_t root = {.uri = "/",
                      .method = HTTP_GET,
                      .handler = root_handler,
                      .user_ctx = NULL};
  httpd_register_uri_handler(server, &root);
  httpd_uri_t css = {.uri = "/style.css",
                     .method = HTTP_GET,
                     .handler = css_handler,
                     .user_ctx = NULL};
  httpd_register_uri_handler(server, &css);
  httpd_uri_t js = {.uri = "/script.js",
                    .method = HTTP_GET,
                    .handler = js_handler,
                    .user_ctx = NULL};
  httpd_register_uri_handler(server, &js);
  httpd_uri_t set = {.uri = "/relay",
                     .method = HTTP_GET,
                     .handler = relay_handler,
                     .user_ctx = NULL};
  httpd_register_uri_handler(server, &set);
httpd_uri_t get = {
    .uri = "/get",
    .method = HTTP_GET,
    .handler = get_handler,
    .user_ctx = NULL
};
httpd_register_uri_handler(server, &get);
  ESP_LOGI(TAG, "HTTP server started");
}