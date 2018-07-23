#pragma once
#include <stdint.h>

typedef struct {
  size_t element_size;
  size_t l;
  size_t *indexes_server;
  uint8_t *shares_server; // t values corresponding to indexes, serialized
  uint8_t *key_client;
} zero_sharing_oblivc_args;

void zero_sharing_oblivc(void *args);
