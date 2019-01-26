#pragma once
#include <stdint.h>

typedef struct {
  size_t element_size;
  size_t num_ciphertexts;
  uint8_t *indexes_server;
  uint8_t *values;
  uint8_t *ciphertexts_server;  // t values corresponding to indexes, serialized
  uint8_t *key_client;
  uint8_t *result_server;
} zero_sharing_oblivc_args;

void zero_sharing_oblivc(void *args);
