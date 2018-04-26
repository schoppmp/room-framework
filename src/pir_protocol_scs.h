#pragma once
#include <stdint.h>

typedef struct {
  size_t key_type_size;
  size_t value_type_size;
  size_t num_elements;
  uint8_t *input;
  uint8_t *result;
} pir_scs_oblivc_args;

void pir_scs_oblivc(void *args);
