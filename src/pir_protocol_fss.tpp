#include "serialize.h"
extern "C" {
  #include <obliv.h>
  #include "pir_protocol_fss.h"
}

template<typename K, typename V>
void pir_protocol_fss<K,V>::run_server(
  const pir_protocol_fss<K,V>::PairIterator input_first, size_t input_length,
  const pir_protocol_fss<K,V>::ValueIterator default_first, size_t default_length
) {
  // set up obliv-c inputs
  std::vector<uint8_t> input, defaults_bytes(default_length * sizeof(V));
  serialize_le(defaults_bytes.data(), default_first, default_length);
  input.reserve(256);
  size_t max_key = 0;
  auto it = input_first;
  for(size_t i = 0; i < input_length; i++, it++) {
    // write values into a dense vector
    K current_key = it->first;
    while(input.capacity() <= current_key) {
      input.reserve(2*input.capacity());
    }
    if(current_key > max_key) {
      max_key = current_key;
      input.resize((current_key + 1) * (sizeof(V)+1), 0);
    }
    serialize_le(&input[current_key * (sizeof(V) + 1)], &(it->second), 1);
    // one extra byte per element to distinguish missing values from zeros
    input[current_key * (sizeof(V) + 1) + sizeof(V)] = 1;
  }
  pir_fss_oblivc_args args = {
    .value_type_size = sizeof(V),
    .num_server_values = max_key + 1,
    .server_values = input.data(),
    .server_defaults = defaults_bytes.data(),
    .num_client_keys = default_length,
    .client_keys = nullptr,
    .result = nullptr,
  };

  // run yao's protocol using Obliv-C
  ProtocolDesc pd;
  if(chan.connect_to_oblivc(pd) == -1) {
    BOOST_THROW_EXCEPTION(std::runtime_error("run_server: connection failed"));
  }
  setCurrentParty(&pd, 1);
  execYaoProtocol(&pd, pir_fss_oblivc, &args);
  cleanupProtocol(&pd);
}

template<typename K, typename V>
void pir_protocol_fss<K,V>::run_client(
  const pir_protocol_fss<K,V>::KeyIterator input_first,
  pir_protocol_fss<K,V>::ValueIterator output_first, size_t length
) {
  // set up obliv-c inputs
  std::vector<size_t> input(length); // FLORAM assumes size_t as indexes
  std::vector<uint8_t> output(length * sizeof(V));
  std::copy_n(input_first, length, input.begin());
  pir_fss_oblivc_args args = {
    .value_type_size = sizeof(V),
    .num_server_values = 0,
    .server_values = nullptr,
    .server_defaults = nullptr,
    .num_client_keys = length,
    .client_keys = input.data(),
    .result = output.data()
  };

  // run yao's protocol using Obliv-C
  ProtocolDesc pd;
  if(chan.connect_to_oblivc(pd) == -1) {
    BOOST_THROW_EXCEPTION(std::runtime_error("run_client: connection failed"));
  }
  setCurrentParty(&pd, 2);
  execYaoProtocol(&pd, pir_fss_oblivc, &args);
  cleanupProtocol(&pd);

  deserialize_le(output_first, output.data(), length);
}
