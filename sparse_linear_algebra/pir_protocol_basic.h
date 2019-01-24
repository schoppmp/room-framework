#pragma once
#include <stdint.h>

typedef struct {
  size_t index_size;
  size_t element_size;
  size_t num_ciphertexts;
  uint8_t *indexes_client;
  uint8_t *ciphertexts_client;
  uint8_t *defaults_server;
  uint8_t *key_server;
  uint8_t *result;
  bool shared_output;
} pir_basic_oblivc_args;

void pir_basic_oblivc(void *args);
