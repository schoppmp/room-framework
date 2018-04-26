#pragma once
#include <stdint.h>

// quick-and-dirty little-endian serialization of arbitrary integer types
// assumes `in` is an array of unsigned integers of at most `element_size` bytes,
// `out` is a byte array, and n is the number of elements in `in`.
// Works with Obliv-C's `obliv` types.
#define SERIALIZE(out, in, n) do { \
  for(size_t i = 0; i < n; i++) { \
    size_t element_size = sizeof(*in) / sizeof(*out); \
    for(size_t j = 0; j < element_size; j++) { \
      *(out + i*sizeof(*in)+j) = (typeof(*out))(*(in + i) >> (8 * j)); \
    } \
  } \
} while(0)

#define DESERIALIZE(out, in, n) do { \
  for(size_t i = 0; i < n; i++) { \
    *(out + i) = 0; \
    size_t element_size = sizeof(*out) / sizeof(*in); \
    for(size_t j = 0; j < element_size; j++) { \
      *(out + i) ^= ((typeof(*out))(*(in + i*sizeof(*out)+j)) << (8 * j)); \
    } \
  } \
} while(0)
