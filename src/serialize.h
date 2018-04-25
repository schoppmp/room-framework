#pragma once
#include <stdint.h>

// quick-and-dirty little-endian serialization of arbitrary integer types
// assumes `in` is an array of unsigned integers of any size, `out` is a byte array,
// and n is the number of elements in `in`.
#define SERIALIZE(out, in, n) do { \
  for(size_t i = 0; i < n; i++) { \
    for(size_t j = 0; j < sizeof(*in); j++) { \
      *(out + i*sizeof(*in)+j) = uint8_t(*(in + i) >> (8 * j)); \
    } \
  } \
} while(0)

#define DESERIALIZE(out, in, n) do { \
  for(size_t i = 0; i < n; i++) { \
    *(out + i) = 0; \
    for(size_t j = 0; j < sizeof(*out); j++) { \
      *(out + i) |= ((typeof(*out))(*(in + i*sizeof(*out)+j)) << (8 * j)); \
    } \
  } \
} while(0)
