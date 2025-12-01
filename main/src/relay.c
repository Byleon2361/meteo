// #include <stdio.h>

// #include "driver/gpio.h"
// #include "esp_err.h"
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "i2c_bus.h"

// #define BOOT_BUTTON_GPIO 0

// i2c_bus_handle_t i2c0_bus = NULL;

// QueueHandle_t button_queue = NULL;

// typedef enum { BUTTON_PRESSED } button_event_t;

// static void IRAM_ATTR button_handler(void* arg) {
//   static uint32_t last_isr_time = 0;
//   uint32_t now = xTaskGetTickCountFromISR();
//   button_event_t event = BUTTON_PRESSED;

//   if ((now - last_isr_time) * portTICK_PERIOD_MS > 150) {
//     xQueueSendFromISR(button_queue, &event, NULL);
//     last_isr_time = now;
//   }
// }

// void init_gpio(void) {
//   gpio_config_t button = {.pin_bit_mask = (1ULL << BOOT_BUTTON_GPIO),
//                           .mode = GPIO_MODE_INPUT,
//                           .pull_up_en = GPIO_PULLUP_ENABLE,
//                           .pull_down_en = GPIO_PULLDOWN_DISABLE,
//                           .intr_type = GPIO_INTR_NEGEDGE};
//   gpio_config(&button);

//   gpio_install_isr_service(ESP_INTR_FLAG_IRAM);
//   gpio_isr_handler_add(BOOT_BUTTON_GPIO, button_handler, NULL);
//   gpio_intr_enable(BOOT_BUTTON_GPIO);
// }

// void init_i2c(void) {
//   i2c_config_t conf = {
//       .mode = I2C_MODE_MASTER,
//       .sda_io_num = GPIO_NUM_21,
//       .sda_pullup_en = GPIO_PULLUP_ENABLE,
//       .scl_io_num = GPIO_NUM_22,
//       .scl_pullup_en = GPIO_PULLUP_ENABLE,
//       .master.clk_speed = 100000,
//   };

//   i2c0_bus = i2c_bus_create(I2C_NUM_0, &conf);
//   if (i2c0_bus == NULL) {
//     printf("Failed to create I2C bus\n");
//     return;
//   }
// }

// void print_i2c_address(void) {
//   uint8_t addresses[127];
//   int count = i2c_bus_scan(i2c0_bus, addresses, 127);

//   printf("Scanning the bus...\r\n\r\n");
//   printf("     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f\r\n");

//   for (int i = 0; i < 128; i += 16) {
//     printf("%02x: ", i);
//     for (int j = 0; j < 16; j++) {
//       fflush(stdout);
//       int k;
//       for (k = 0; k < count; k++) {
//         if (addresses[k] == i + j) {
//           printf("%02x ", addresses[k]);
//           break;
//         }
//       }
//       if (k == count) {
//         printf("-- ");
//       }
//     }
//     printf("\r\n");
//   }
// }

// void button_task() {
//   button_event_t event;
//   while (1) {
//     if (xQueueReceive(button_queue, &event, portMAX_DELAY) == pdTRUE) {
//       print_i2c_address();
//     }
//   }
// }

// void app_main(void) {
//   button_queue = xQueueCreate(10, sizeof(button_event_t));
//   if (button_queue == NULL) {
//     printf("Error create queue\n");
//     return;
//   }

//   printf("Init gpio\n");
//   init_gpio();
//   printf("Init i2c\n");
//   init_i2c();

//   xTaskCreate(button_task, "button_task", 4096, NULL, 10, NULL);
//   printf("Press BOOT button for print i2c addresses\n");

//   while (1) {
//     vTaskDelay(pdMS_TO_TICKS(1000));
//   }
// }