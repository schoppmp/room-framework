#include <stdint.h>
#include <stddef.h>

struct BenchmarkFSSArgs {
  size_t len;
};

void benchmark_fss(void *args);
