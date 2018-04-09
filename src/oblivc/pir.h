#pragma once
#include <stdint.h>

typedef uint32_t pir_value;

typedef struct {
  size_t input_length;
  uint8_t *input; // either encrypted elements or the key
  pir_value *result;
} pir_args;

void pir_protocol_main(void *args);
