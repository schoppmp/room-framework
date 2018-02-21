#pragma once
#include <stdint.h>
#include <stddef.h>

struct BenchmarkSCSArgs {
  const size_t len; // number of elements
  const uint32_t *values; // `len` sorted values
  double result_time;
};

void benchmark_scs(void *vargs);
