#pragma once

#include <vector>
#include <map>

// A (sparse) PIR protocol is executed between two parties, called Server and
// Client. The server inputs a map from keys to values. The client inputs a
// selection vector of keys. The result is a vector of additively shared values.
// For each index i of the resulting vectors,
// server_out[i] + client_out[i] = server_in[client_in[i]],
// where missing elements in server_in get the value 0.
template<typename K, typename V>
class pir_protocol {
public:
  using key_type = K;
  using value_type = V;

  virtual void run_server(const std::map<K,V>& server_in, std::vector<V>& server_out) = 0;
  virtual void run_client(const std::vector<K>& client_in, std::vector<V>& client_out) = 0;
  std::vector<V> run_server(const std::map<K,V>& server_in);
  std::vector<V> run_client(const std::vector<K>& client_in);
};

#include "pir_protocol.tpp"
