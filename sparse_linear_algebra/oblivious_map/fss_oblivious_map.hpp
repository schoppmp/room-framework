#pragma once

#include <type_traits>
#include "oblivious_map.hpp"
#include "mpc_utils/comm_channel.hpp"

template<typename K, typename V>
class fss_oblivious_map : public virtual oblivious_map<K,V> {
static_assert(std::is_integral<K>::value, "FSS-based PIR only works for integral key types");

private:
  comm_channel& chan;
  bool print_times;
  bool cprg;

public:
  fss_oblivious_map(comm_channel& chan, bool cprg = false, bool print_times = false) :
    chan(chan), cprg(cprg), print_times(print_times) { }
  ~fss_oblivious_map() { }

  using pair_range = typename oblivious_map<K, V>::pair_range;
  using key_range = typename oblivious_map<K, V>::key_range;
  using value_range = typename oblivious_map<K, V>::value_range;

  void run_server(const pair_range input, const value_range defaults,
    bool shared_output);
  void run_client(const key_range input, value_range output,
    bool shared_output);
};

#include "fss_oblivious_map.tpp"
