#pragma once
#include <vector>

template <typename T>
const T& get_ceil(const std::vector<T>& v, size_t i) {
  return i < v.size() ? v[i] : v[v.size() - 1];
}