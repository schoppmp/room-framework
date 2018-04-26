#pragma once
#include <stdint.h>

typedef struct {
  size_t value_type_size;
  size_t num_server_values;
  uint8_t *server_values;
  size_t num_client_keys;
  size_t *client_keys;
  uint8_t *result;
} pir_fss_oblivc_args;

void pir_fss_oblivc(void *args);
