#pragma once
#include <time.h>

#ifdef __cplusplus
#include <chrono>
// used for time measurements
template<typename F>
void benchmark(F f, const std::string& label) {
  auto start = std::chrono::steady_clock::now();
  f();
  std::chrono::duration<double> d = std::chrono::steady_clock::now() - start;
  std::cout << label << ": " << d.count() << " s\n";
}
#endif

// returns the current time; use for benchmarks
double timestamp() {
  struct timespec t;
  clock_gettime(CLOCK_MONOTONIC,&t);
  return t.tv_sec+1e-9*t.tv_nsec;
}
