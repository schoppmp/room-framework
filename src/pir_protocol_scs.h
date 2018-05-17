#pragma once
#include <stdint.h>

typedef struct {
  size_t key_type_size;
  size_t value_type_size;
  size_t num_elements;
  const uint8_t *input_keys;
  const uint8_t *input_values;
  const uint8_t *input_defaults;
  uint8_t *result_keys;
  uint8_t *result_values;
  bool shared_output;
} pir_scs_oblivc_args;

void pir_scs_oblivc(void *args);
