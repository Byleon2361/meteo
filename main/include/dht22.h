#pragma once
#include <stdint.h>

#define DHT_OK 0
#define DHT_TIMEOUT 1
#define DHT_CHECKSUM_FAIL 2

typedef struct {
  uint8_t gpio;
  uint64_t last_read_time;
  uint8_t data[5];
} dht22_t;

typedef struct {
  uint8_t gpio;
} dht_params_data_t;

void dht22_task(void *);