#include "serialize.h"
extern "C" {
  #include <obliv.h>
  #include "pir_protocol_fss.h"
}


template<typename K, typename V>
void pir_protocol_fss<K, V>::run_server(const std::map<K,V>& server_in, std::vector<V>& server_out) {
  // flatten input map to a vector
  size_t max_key = std::accumulate(server_in.begin(), server_in.end(), 0,
    [](const size_t& a, const std::pair<K,V>& b) {
      return std::max(a, size_t(b.first));
    });
  size_t num_elements_client;
  chan.recv(num_elements_client);
  std::vector<uint8_t> input((max_key + 1) * sizeof(V), 0), output(num_elements_client * sizeof(V));
  for(auto& pair : server_in) {
    // server_in_vec[pair.first] = pair.second;
    SERIALIZE(&input[sizeof(V) * pair.first], &pair.second, 1);
  }
  pir_fss_oblivc_args args = {
    .key_type_size = sizeof(K),
    .value_type_size = sizeof(V),
    .input_size = input.size(),
    .input = input.data(),
    .result = output.data()
  };
  // TODO

  server_out.resize(num_elements_client);
  DESERIALIZE(server_out.data(), output.data(), server_out.size());
}

template<typename K, typename V>
void pir_protocol_fss<K, V>::run_client(const std::vector<K>& client_in, std::vector<V>& client_out) {
  chan.send(client_in.size());
  size_t input_size = client_in.size() * sizeof(K);
  size_t output_size = client_in.size() * sizeof(V);
  std::vector<uint8_t> input(input_size), output(output_size);
  SERIALIZE(input.data(), client_in.data(), client_in.size());
  pir_fss_oblivc_args args = {
    .key_type_size = sizeof(K),
    .value_type_size = sizeof(V),
    .input_size = input.size(),
    .input = input.data(),
    .result = output.data()
  };
  // TODO

  client_out.resize(client_in.size());
  DESERIALIZE(client_out.data(), output.data(), client_out.size());
}
