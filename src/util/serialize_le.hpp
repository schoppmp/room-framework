#pragma once
#include <stdint.h>

// quick-and-dirty little-endian serialization of arbitrary integer types
// assumes `in` is an array of unsigned integers of at most `element_size` bytes,
// `out` is a byte array, and n is the number of elements in `in`.
template<typename InputIterator, typename OutputIterator>
void serialize_le(OutputIterator out, InputIterator in, size_t n) {
  size_t element_size = sizeof(*in) / sizeof(*out);
  // std::cout << "element_size: " << element_size << "\n";
  for(size_t i = 0; i < n; i++, in++) {
    auto _x = *out;  // Apparently needed as decltype(*out) does not work below.
    for(size_t j = 0; j < element_size; j++, out++) {
      *out = (decltype(_x)) (*in >> (8 * j));
    }
  }
}

template<typename InputIterator, typename OutputIterator>
void deserialize_le(OutputIterator out, InputIterator in, size_t n) {
  size_t element_size = sizeof(*out) / sizeof(*in);
  for(size_t i = 0; i < n; i++, out++) {
    *out = 0;
    auto _x = *out;
    for(size_t j = 0; j < element_size; j++, in++) {
      *out ^= ((decltype(_x)) *in) << (8 * j);
    }
  }
}
