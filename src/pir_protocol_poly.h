#pragma once
#include <stdint.h>

typedef struct {
  size_t statistical_security;
  size_t value_type_size;
  size_t input_size;
  const uint8_t *input; // either encrypted elements or the key
  const uint8_t *defaults;
  uint8_t *result;
} pir_poly_oblivc_args;

void pir_poly_oblivc(void *args);
