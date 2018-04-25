#pragma once

#include "pir_protocol.hpp"
#include "mpc-utils/comm_channel.hpp"

template<typename K, typename V>
class pir_protocol_fss : public virtual pir_protocol<K,V> {
private:
  comm_channel& chan;

public:
  pir_protocol_poly(comm_channel& chan) : chan(chan) { }

  using pir_protocol<K, V>::run_server;
  using pir_protocol<K, V>::run_client;
  void run_server(const std::map<K,V>& server_in, std::vector<V>& server_out);
  void run_client(const std::vector<K>& client_in, std::vector<V>& client_out);
};

#include "pir_protocol_fss.tpp"
