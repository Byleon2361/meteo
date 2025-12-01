#include "relay.h"

#include <stdlib.h>
#include "esp_log.h"
#include "driver/gpio.h"
static uint8_t relay_gpio = 0;
void relay_on()
{
        gpio_set_level(relay_gpio, 1);
}
void relay_off()
{
        gpio_set_level(relay_gpio, 0);
}
void init_relay(uint8_t gpio) 
{
    relay_gpio = gpio;
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << gpio),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
    
    // Изначально выключаем реле
    gpio_set_level(gpio, 0);
    ESP_LOGI("RELAY", "Реле инициализировано на GPIO 2 (ВЫКЛ)");
}