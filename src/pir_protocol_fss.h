#pragma once
#include <stdint.h>

typedef struct {
  size_t key_type_size;
  size_t value_type_size;
  size_t input_size;
  uint8_t *input;
  uint8_t *result;
} pir_fss_oblivc_args;

void pir_fss_oblivc(void *args);
