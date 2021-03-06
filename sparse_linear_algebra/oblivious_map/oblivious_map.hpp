#pragma once

#include <map>
#include <vector>
#include "any_iterator/any_iterator.hpp"
#include "boost/range/any_range.hpp"
#include "mpc_utils/benchmarker.hpp"

// A (sparse) PIR protocol is executed between two parties, called Server and
// Client. The server inputs an iterator over key-value pairs, and an iterator
// of default values. The client inputs a selection vector of keys. The result
// is a vector of values, where client_out[i] = server_in[client_in[i]] if
// client_in[i] exists in server_in, and client_out[i] = defaults[i] otherwise
template <typename K, typename V>
class oblivious_map {
 public:
  using key_type = K;
  using value_type = V;
  virtual ~oblivious_map(){};

  // Range types for container-agnostic protocols
  using key_range =
      boost::any_range<K, boost::single_pass_traversal_tag, K&, std::ptrdiff_t>;
  using value_range =
      boost::any_range<V, boost::single_pass_traversal_tag, V&, std::ptrdiff_t>;
  // we cannot use any_range here, since it does not use conversions to build
  // reference types, which is needed to capture both zip_iterators and
  // std::map::iterators under the same type
  using pair_range = boost::iterator_range<IteratorTypeErasure::any_iterator<
      std::pair<const K, const V>, boost::single_pass_traversal_tag,
      std::pair<const K&, const V&>, std::ptrdiff_t>>;

  // most generic functions to be implemented by subclasses
  virtual void run_server(const pair_range input, const value_range defaults,
                          bool shared_output = false,
                          mpc_utils::Benchmarker* benchmarker = nullptr) = 0;
  virtual void run_client(const key_range input, value_range output,
                          bool shared_output = false,
                          mpc_utils::Benchmarker* benchmarker = nullptr) = 0;

  // adapter for separate key and value ranges; can be overloaded by subclasses
  // but also provides a default implementation
  virtual void run_server(const key_range input_keys,
                          const value_range input_values,
                          const value_range defaults,
                          bool shared_output = false,
                          mpc_utils::Benchmarker* benchmarker = nullptr);

  // adapter for converting an any_range of pairs to a pair_range
  void run_server(
      boost::any_range<std::pair<const K, V>, boost::single_pass_traversal_tag,
                       std::pair<const K, V>&, boost::use_default>
          input,
      value_range defaults, bool shared_output = false,
      mpc_utils::Benchmarker* benchmarker = nullptr);
};

#include "oblivious_map.tpp"
