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

  using pair_range = typename pir_protocol<K, V>::pair_range;
  using key_range = typename pir_protocol<K, V>::key_range;
  using value_range = typename pir_protocol<K, V>::value_range;

  void run_server(const pair_range input, const value_range defaults);
  void run_client(const key_range input, const value_range output);
};

#include "pir_protocol_fss.tpp"
