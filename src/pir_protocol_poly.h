#pragma once
#include <stdint.h>

typedef uint64_t pir_poly_value;

typedef struct {
  size_t statistical_security;
  size_t input_size;
  uint8_t *input; // either encrypted elements or the key
  pir_poly_value *result;
} pir_poly_oblivc_args;

void pir_poly_oblivc(void *args);
