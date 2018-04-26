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
  size_t num_client_keys;
  chan.recv(num_client_keys);

  // set up obliv-c inputs
  std::vector<uint8_t> input((max_key + 1) * sizeof(V), 0),
    output(num_client_keys * sizeof(V));
  for(auto& pair : server_in) {
    SERIALIZE(&input[sizeof(V) * pair.first], &pair.second, 1);
  }
  pir_fss_oblivc_args args = {
    .value_type_size = sizeof(V),
    .num_server_values = max_key + 1,
    .server_values = input.data(),
    .num_client_keys = num_client_keys,
    .client_keys = nullptr,
    .result = output.data()
  };

  // run yao's protocol using Obliv-C
  ProtocolDesc pd;
  chan.flush();
  if(chan.connect_to_oblivc(pd) == -1) {
    BOOST_THROW_EXCEPTION(std::runtime_error("run_server: connection failed"));
  }
  setCurrentParty(&pd, 1);
  execYaoProtocol(&pd, pir_fss_oblivc, &args);
  cleanupProtocol(&pd);

  server_out.resize(num_client_keys);
  DESERIALIZE(server_out.data(), output.data(), server_out.size());
}

template<typename K, typename V>
void pir_protocol_fss<K, V>::run_client(const std::vector<K>& client_in, std::vector<V>& client_out) {
  chan.send(client_in.size());
  size_t output_size = client_in.size() * sizeof(V);

  // set up obliv-c inputs
  std::vector<size_t> input(client_in.size()); // FLORAM assumes size_t as indexes
  std::vector<uint8_t> output(output_size);
  std::copy(client_in.begin(), client_in.end(), input.begin());
  pir_fss_oblivc_args args = {
    .value_type_size = sizeof(V),
    .num_server_values = 0,
    .server_values = nullptr,
    .num_client_keys = client_in.size(),
    .client_keys = input.data(),
    .result = output.data()
  };

  // run yao's protocol using Obliv-C
  ProtocolDesc pd;
  chan.flush();
  if(chan.connect_to_oblivc(pd) == -1) {
    BOOST_THROW_EXCEPTION(std::runtime_error("run_client: connection failed"));
  }
  setCurrentParty(&pd, 2);
  execYaoProtocol(&pd, pir_fss_oblivc, &args);
  cleanupProtocol(&pd);

  client_out.resize(client_in.size());
  DESERIALIZE(client_out.data(), output.data(), client_out.size());
}
