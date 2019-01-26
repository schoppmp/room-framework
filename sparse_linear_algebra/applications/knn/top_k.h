#pragma once
#include <stdint.h>

typedef struct {
  const int num_elements;
  const int num_selected;
  const int value_size;
  const uint8_t* serialized_inputs;
  const uint8_t* serialized_norms;
  int* result;
} knn_oblivc_args;

void top_k_oblivc(void* vargs);
