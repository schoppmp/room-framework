#include "serialize.h"
extern "C" {
  #include <obliv.h>
  #include "pir_protocol_scs.h"
}

template<typename K, typename V>
void pir_protocol_scs<K, V>::run_server(const std::map<K,V>& server_in, std::vector<V>& server_out) {
  size_t num_client_keys;
  chan.recv(num_client_keys);

  pir_scs_oblivc_args args = {
    .key_type_size = sizeof(K),
    .value_type_size = sizeof(V),
    .num_elements = server_in.size(),
    .input = nullptr, // TODO
    .result = nullptr
  };
}

template<typename K, typename V>
void pir_protocol_scs<K, V>::run_client(const std::vector<K>& client_in, std::vector<V>& client_out) {
  chan.send(client_in.size());

  pir_scs_oblivc_args args = {
    .key_type_size = sizeof(K),
    .value_type_size = sizeof(V),
    .num_elements = client_in.size(),
    .input = nullptr, // TODO
    .result = nullptr
  };
}
