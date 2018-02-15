#include <stdint.h>
#include <stddef.h>

struct BenchmarkFSSArgs {
  size_t len;
  size_t num_iterations;
  double result_time;
};

void benchmark_fss(void *args);
