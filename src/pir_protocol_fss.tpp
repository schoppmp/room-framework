#include "utils.h"
extern "C" {
  #include <obliv.h>
  #include "pir_protocol_fss.h"
}

template<typename K, typename V>
void pir_protocol_fss<K,V>::run_server(
  const pir_protocol_fss<K,V>::pair_range input,
  const pir_protocol_fss<K,V>::value_range defaults,
  bool shared_output
) {
  double local_time = 0, mpc_time = 0;
  double start = timestamp(), end;
  size_t input_length = boost::size(input);
  size_t default_length = boost::size(defaults);
  // set up obliv-c inputs
  std::vector<uint8_t> input_bytes, defaults_bytes(default_length * sizeof(V));
  serialize_le(defaults_bytes.data(), std::begin(defaults), default_length);
  input_bytes.reserve(256);
  size_t max_key = 0;
  auto it = boost::begin(input);
  for(size_t i = 0; i < input_length; i++, it++) {
    // write values into a dense vector
    K current_key = it->first;
    while(input_bytes.capacity() <= current_key * (sizeof(V) + 1)) {
      input_bytes.reserve(2*input_bytes.capacity());
    }
    if(current_key > max_key) {
      max_key = current_key;
      input_bytes.resize((current_key + 1) * (sizeof(V)+1), 0);
    }
    serialize_le(&input_bytes[current_key * (sizeof(V) + 1)], &(it->second), 1);
    // one extra byte per element to distinguish missing values from zeros
    input_bytes[current_key * (sizeof(V) + 1) + sizeof(V)] = 1;
  }
  pir_fss_oblivc_args args = {
    .value_type_size = sizeof(V),
    .num_server_values = max_key + 1,
    .server_values = input_bytes.data(),
    .server_defaults = defaults_bytes.data(),
    .num_client_keys = default_length,
    .client_keys = nullptr,
    .result = nullptr,
    .local_time = 0.
  };
  end = timestamp();
  local_time += end - start;
  start = end;

  // run yao's protocol using Obliv-C
  ProtocolDesc pd;
  if(chan.connect_to_oblivc(pd) == -1) {
    BOOST_THROW_EXCEPTION(std::runtime_error("run_server: connection failed"));
  }
  setCurrentParty(&pd, 1);
  execYaoProtocol(&pd, pir_fss_oblivc, &args);
  cleanupProtocol(&pd);
  end = timestamp();
  mpc_time += end - start - args.local_time;
  local_time += args.local_time;
  if(print_times) {
    std::cout << "local_time: " << local_time << " s\n";
    std::cout << "mpc_time: " << mpc_time << " s\n";
  }
}

template<typename K, typename V>
void pir_protocol_fss<K,V>::run_client(
  const pir_protocol_fss<K,V>::key_range input,
  const pir_protocol_fss<K,V>::value_range output,
  bool shared_output
) {
  double local_time = 0, mpc_time = 0;
  double start = timestamp(), end;
  size_t length = boost::size(input);
  // set up obliv-c inputs
  std::vector<size_t> input_size_t(length); // FLORAM assumes size_t as indexes
  std::vector<uint8_t> output_bytes(length * sizeof(V));
  boost::copy(input, input_size_t.begin());
  pir_fss_oblivc_args args = {
    .value_type_size = sizeof(V),
    .num_server_values = 0,
    .server_values = nullptr,
    .server_defaults = nullptr,
    .num_client_keys = length,
    .client_keys = input_size_t.data(),
    .result = output_bytes.data(),
    .local_time = 0.
  };
  end = timestamp();
  local_time += end - start;
  start = end;

  // run yao's protocol using Obliv-C
  ProtocolDesc pd;
  if(chan.connect_to_oblivc(pd) == -1) {
    BOOST_THROW_EXCEPTION(std::runtime_error("run_client: connection failed"));
  }
  setCurrentParty(&pd, 2);
  execYaoProtocol(&pd, pir_fss_oblivc, &args);
  cleanupProtocol(&pd);

  end = timestamp();
  mpc_time += end - start - args.local_time;
  local_time += args.local_time;
  start = end;
  deserialize_le(output.begin(), output_bytes.data(), length);
  end = timestamp();
  local_time += end - start;
  if(print_times) {
    std::cout << "local_time: " << local_time << " s\n";
    std::cout << "mpc_time: " << mpc_time << " s\n";
  }
}
