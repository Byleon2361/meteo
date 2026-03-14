#include "tunnel.h"

#include <string.h>

#include "esp_log.h"
#include "esp_netif.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"

static const char* TAG = "TUNNEL";

#define VPS_HOST "72.56.93.56"
#define VPS_TUNNEL_PORT 9000
#define LOCAL_WEB_PORT 80
#define POOL_SIZE 6

static uint32_t get_local_ip(void)
{
    esp_netif_t* netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (!netif)
        return 0;
    esp_netif_ip_info_t ip_info;
    if (esp_netif_get_ip_info(netif, &ip_info) != ESP_OK)
        return 0;
    return ip_info.ip.addr;
}

static void tunnel_worker(void* arg)
{
    while (1) {
        int vps_sock = socket(AF_INET, SOCK_STREAM, 0);
        if (vps_sock < 0) {
            vTaskDelay(pdMS_TO_TICKS(3000));
            continue;
        }

        struct sockaddr_in vps_addr = {
                .sin_family = AF_INET,
                .sin_port = htons(VPS_TUNNEL_PORT),
        };
        inet_pton(AF_INET, VPS_HOST, &vps_addr.sin_addr);

        if (connect(vps_sock, (struct sockaddr*)&vps_addr, sizeof(vps_addr))
            != 0) {
            ESP_LOGW(TAG, "Не удалось подключиться к VPS, повтор через 5с");
            close(vps_sock);
            vTaskDelay(pdMS_TO_TICKS(5000));
            continue;
        }
        ESP_LOGI(TAG, "Соединение с VPS установлено");

        char request[2048] = {0};
        struct timeval tv_req = {.tv_sec = 60, .tv_usec = 0};
        setsockopt(vps_sock, SOL_SOCKET, SO_RCVTIMEO, &tv_req, sizeof(tv_req));

        int req_len = recv(vps_sock, request, sizeof(request) - 1, 0);
        if (req_len <= 0) {
            ESP_LOGW(TAG, "Нет запроса от VPS");
            close(vps_sock);
            continue;
        }

        uint32_t local_ip = get_local_ip();
        if (local_ip == 0) {
            ESP_LOGE(TAG, "Не удалось получить локальный IP");
            close(vps_sock);
            continue;
        }

        int local_sock = socket(AF_INET, SOCK_STREAM, 0);
        struct sockaddr_in local_addr = {
                .sin_family = AF_INET,
                .sin_port = htons(LOCAL_WEB_PORT),
                .sin_addr.s_addr = local_ip, // 192.168.0.107 вместо 127.0.0.1
        };

        if (connect(local_sock,
                    (struct sockaddr*)&local_addr,
                    sizeof(local_addr))
            == 0) {
            send(local_sock, request, req_len, 0);

            char response[4096];
            int resp_len;
            struct timeval tv = {.tv_sec = 5, .tv_usec = 0};
            setsockopt(local_sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

            while ((resp_len = recv(local_sock, response, sizeof(response), 0))
                   > 0) {
                send(vps_sock, response, resp_len, 0);
            }
            ESP_LOGI(TAG, "Запрос проксирован успешно");
        } else {
            ESP_LOGE(TAG, "Не удалось подключиться к webserver");
        }

        close(local_sock);
        close(vps_sock);
    }
}

void tunnel_init(void)
{
    for (int i = 0; i < POOL_SIZE; i++) {
        xTaskCreate(tunnel_worker, "tunnel_worker", 8192, NULL, 3, NULL);
        vTaskDelay(pdMS_TO_TICKS(200));
    }
    ESP_LOGI(TAG, "Туннель запущен (%d соединений)", POOL_SIZE);
}
