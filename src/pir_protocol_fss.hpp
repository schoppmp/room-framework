#pragma once

#include <type_traits>
#include "pir_protocol.hpp"
#include "mpc-utils/comm_channel.hpp"

template<typename K, typename V>
class pir_protocol_fss : public virtual pir_protocol<K,V> {
static_assert(std::is_integral<K>::value, "FSS-based PIR only works for integral key types");

private:
  comm_channel& chan;

public:
  pir_protocol_fss(comm_channel& chan) : chan(chan) { }
  ~pir_protocol_fss() { }

  using pir_protocol<K, V>::run_server;
  using pir_protocol<K, V>::run_client;
  using PairIterator = typename pir_protocol<K, V>::PairIterator;
  using KeyIterator = typename pir_protocol<K, V>::KeyIterator;
  using ValueIterator = typename pir_protocol<K, V>::ValueIterator;

  void run_server(const PairIterator input_first, size_t input_length,
    const ValueIterator default_first, size_t default_length);
  void run_client(const KeyIterator input_first, ValueIterator output_first,
    size_t length);
};

#include "pir_protocol_fss.tpp"
