#pragma once
#include <vector>
#include <random>
#include <boost/throw_exception.hpp>

// sample k indices from range [0, max) using reservoir sampling
template<typename T, class Generator>
std::vector<T> reservoir_sampling(Generator&& r, T k, T max) {
    using error_k = boost::error_info<struct tag_sample_K,T>;
    using error_max = boost::error_info<struct tag_sample_MAX,T>;
    if(k > max) {
      BOOST_THROW_EXCEPTION(boost::enable_error_info(std::invalid_argument(
          "`k` must be less than or equal to `max`")) << error_k(k) << error_max(max));
    }
    std::vector<T> a(k);
    for(T i = 0; i < max; i++) {
        T j = std::uniform_int_distribution<T>(0, i)(r);
        if(i < k && j < i) {
          a[i] = a[j];
        }
        if(j < k) {
          a[j] = i;
        }
    }
    return a;
}
