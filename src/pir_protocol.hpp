#pragma once

#include <vector>
#include <map>
#include "any_iterator.hpp"

// A (sparse) PIR protocol is executed between two parties, called Server and
// Client. The server inputs an iterator over key-value pairs, and an iterator
// of default values. The client inputs a selection vector of keys. The result
// is a vector of values, where client_out[i] = server_in[client_in[i]] if
// client_in[i] exists in server_in, and client_out[i] = defaults[i] otherwise
template<typename K, typename V>
class pir_protocol {
public:
  using key_type = K;
  using value_type = V;
  virtual ~pir_protocol() {};

  // Iterator types for container-agnostic protocols
  using PairIterator = IteratorTypeErasure::any_iterator<std::pair<const K, V>,
    std::forward_iterator_tag>;
    // boost::single_pass_traversal_tag>;
  using KeyIterator = IteratorTypeErasure::any_iterator<K,
    boost::single_pass_traversal_tag>;
  using ValueIterator = IteratorTypeErasure::any_iterator<V,
    boost::single_pass_traversal_tag>;

  // most generic functions to be implemented by subclasses
  virtual void run_server(const PairIterator input_first, size_t input_length,
    const ValueIterator default_first, size_t default_length) = 0;
  virtual void run_client(const KeyIterator input_first,
    ValueIterator output_first, size_t length) = 0;

  // templates for converting inputs to any_iterator objects. Needed due to the
  // explicit constructor of any_iterator. See also comment in
  // any_iterator.hpp:147-161
  template<typename Iterator1, typename Iterator2>
  void run_server(const Iterator1 input_first, size_t input_length,
    const Iterator2 default_first, size_t default_length);
  template<typename Iterator1, typename Iterator2>
  void run_client(const Iterator1 input_first,
    const Iterator2 output_first, size_t length);

  // // overloads for concrete containers. can be overloaded by subclasses,
  // // but provide default implementations based on the generic versions above
  // virtual void run_server(const std::map<K,V>& input,
  //   const std::vector<V>& def);
  // virtual void run_server(const std::vector<K>& input_keys,
  //   const std::vector<K>& input_values, const std::vector<V>& def);
  // virtual void run_client(const std::vector<K>& input,
  //   std::vector<V>& client_out);
  // virtual std::vector<V> run_client(const std::vector<K>& input);
  // virtual void run_server(const std::map<K,V>& input, const V& def);
  // virtual void run_client(const std::vector<K>& input, V& out);
  // virtual V run_client(const std::vector<K>& input);
};

#include "pir_protocol.tpp"
