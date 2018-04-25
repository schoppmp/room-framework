#pragma once
#include <stdint.h>

typedef uint64_t pir_fss_value;

typedef struct {
  pir_fss_value *result;
} pir_fss_oblivc_args;

void pir_fss_oblivc(void *args);
