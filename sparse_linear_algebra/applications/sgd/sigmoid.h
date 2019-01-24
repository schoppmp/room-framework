#pragma once
#include <stdint.h>

typedef struct {
  const int num_elements;
  const int element_size;
  const int precision;
  const uint8_t *input;
  uint8_t *output;
} sigmoid_oblivc_args;

void sigmoid_oblivc(void *);
