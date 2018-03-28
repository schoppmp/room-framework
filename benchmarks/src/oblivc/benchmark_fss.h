#pragma once
#include <stdint.h>
#include <stddef.h>

struct BenchmarkFSSArgs {
  const size_t len;
  const size_t num_iterations;
  double result_time;
};

void benchmark_fss(void *args);
