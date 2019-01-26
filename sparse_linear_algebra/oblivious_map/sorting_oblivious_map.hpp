#pragma once

#include "mpc_utils/comm_channel.hpp"
#include "sparse_linear_algebra/oblivious_map/oblivious_map.hpp"

template <typename K, typename V>
class sorting_oblivious_map : public virtual oblivious_map<K, V> {
 private:
  comm_channel& chan;
  bool print_times;

 public:
  sorting_oblivious_map(comm_channel& chan, bool print_times = false)
      : chan(chan), print_times(print_times) {}
  ~sorting_oblivious_map() {}

  using pair_range = typename oblivious_map<K, V>::pair_range;
  using key_range = typename oblivious_map<K, V>::key_range;
  using value_range = typename oblivious_map<K, V>::value_range;

  void run_server(const pair_range input, const value_range defaults,
                  bool shared_output);
  void run_client(const key_range input, value_range output,
                  bool shared_output);
};

#include "sorting_oblivious_map.tpp"
