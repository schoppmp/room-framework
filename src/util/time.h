#pragma once
#include <time.h>

#ifdef __cplusplus

#include <chrono>
#include <iostream>
// used for time measurements
template<typename F>
void benchmark(F f, const std::string& label) {
  auto start = std::chrono::steady_clock::now();
  f();
  std::chrono::duration<double> d = std::chrono::steady_clock::now() - start;
  std::cout << label << ": " << d.count() << " s\n";
}
extern "C" {

#endif

// Returns the current time; use for benchmarks.
double timestamp();

#ifdef __cplusplus
}
#endif
