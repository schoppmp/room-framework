#pragma once
#include <stdint.h>
#include <time.h>

#ifdef __cplusplus
#include <chrono>

// like boost::combine, but with std::pairs
template<typename Range1, typename Range2>
boost::iterator_range<boost::iterators::zip_iterator<std::pair<
  typename Range1::iterator, typename Range2::iterator
>>>
combine_pair(Range1 r1, Range2 r2) {
  auto begin_pair = std::make_pair(boost::begin(r1), boost::begin(r2));
  auto begin_iterator = boost::make_zip_iterator(begin_pair);
  return boost::make_iterator_range_n(begin_iterator, boost::size(r1));
}

// used for time measurements
template<typename F>
void benchmark(F f, const std::string& label) {
  auto start = std::chrono::steady_clock::now();
  f();
  std::chrono::duration<double> d = std::chrono::steady_clock::now() - start;
  std::cout << label << ": " << d.count() << " s\n";
}

// quick-and-dirty little-endian serialization of arbitrary integer types
// assumes `in` is an array of unsigned integers of at most `element_size` bytes,
// `out` is a byte array, and n is the number of elements in `in`.
template<typename InputIterator, typename OutputIterator>
void serialize_le(OutputIterator out, InputIterator in, size_t n) {
  size_t element_size = sizeof(*in) / sizeof(*out);
  // std::cout << "element_size: " << element_size << "\n";
  for(size_t i = 0; i < n; i++, in++) {
    for(size_t j = 0; j < element_size; j++, out++) {
      *out = (typeof(*out)) (*in >> (8 * j));
    }
  }
}

template<typename InputIterator, typename OutputIterator>
void deserialize_le(OutputIterator out, InputIterator in, size_t n) {
  size_t element_size = sizeof(*out) / sizeof(*in);
  for(size_t i = 0; i < n; i++, out++) {
    *out = 0;
    for(size_t j = 0; j < element_size; j++, in++) {
      *out ^= ((typeof(*out)) *in) << (8 * j);
    }
  }
}
#endif

double timestamp() {
  struct timespec t;
  clock_gettime(CLOCK_MONOTONIC,&t);
  return t.tv_sec+1e-9*t.tv_nsec;
}
