#pragma once

#include "pir_protocol.hpp"
#include "mpc_utils/comm_channel.hpp"

template<typename K, typename V>
class pir_protocol_scs : public virtual pir_protocol<K,V> {
private:
  comm_channel& chan;
  bool print_times;

public:
  pir_protocol_scs(comm_channel& chan, bool print_times = false) :
    chan(chan), print_times(print_times) { }
  ~pir_protocol_scs() { }

  using pair_range = typename pir_protocol<K, V>::pair_range;
  using key_range = typename pir_protocol<K, V>::key_range;
  using value_range = typename pir_protocol<K, V>::value_range;

  void run_server(const pair_range input, const value_range defaults,
    bool shared_output);
  void run_client(const key_range input, value_range output,
    bool shared_output);
};

#include "pir_protocol_scs.tpp"
