#pragma once
#include <vector>

template<typename T>
T& get_ceil(std::vector<T>& v, size_t i) {
  return i < v.size() ? v[i] : v[v.size() - 1];
}
