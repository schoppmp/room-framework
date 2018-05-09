#include "utils.h"
extern "C" {
  #include <obliv.h>
  #include "pir_protocol_scs.h"
}
#include <boost/range/adaptor/map.hpp>
#include <boost/range/combine.hpp>
#include <boost/range/irange.hpp>

template<typename K, typename V>
void pir_protocol_scs<K,V>::run_server(
  const pir_protocol_scs<K,V>::pair_range input,
  const pir_protocol_scs<K,V>::value_range defaults
) {
  double local_time = 0, mpc_time = 0;
  double start = timestamp(), end;
  size_t input_size = boost::size(input);
  // sort inputs
  std::vector<std::pair<K,V>> input_vec;
  std::vector<uint8_t> input_keys_bytes(input_size * sizeof(K));
  std::vector<uint8_t> input_values_bytes(input_size * sizeof(V));
  std::vector<uint8_t> input_defaults_bytes(boost::size(defaults) * sizeof(V));
  // benchmark([&]{
  input_vec = std::vector<std::pair<K,V>>(boost::begin(input), boost::end(input));
  boost::sort(input_vec);
  // serialize for obliv-c
  serialize_le(input_keys_bytes.begin(), boost::begin(boost::adaptors::keys(input_vec)), input_size);
  serialize_le(input_values_bytes.begin(), boost::begin(boost::adaptors::values(input_vec)), input_size);
  serialize_le(input_defaults_bytes.begin(), boost::begin(defaults), boost::size(defaults));
  // }, "Copying inputs (server)");
  chan.flush();

  // benchmark([&]{
  pir_scs_oblivc_args args = {
    .key_type_size = sizeof(K),
    .value_type_size = sizeof(V),
    .num_elements = input_vec.size(),
    .input_keys = input_keys_bytes.data(),
    .input_values = input_values_bytes.data(),
    .input_defaults = input_defaults_bytes.data(),
    .result_keys = nullptr,
    .result_values = nullptr
  };
  end = timestamp();
  local_time += end - start;
  start = end;

  // run yao's protocol using Obliv-C
  ProtocolDesc pd;
  if(chan.connect_to_oblivc(pd, 10 /* ms between attempts */) == -1) {
    BOOST_THROW_EXCEPTION(std::runtime_error("run_server: connection failed"));
  }
  setCurrentParty(&pd, 1);
  execYaoProtocol(&pd, pir_scs_oblivc, &args);
  cleanupProtocol(&pd);

  end = timestamp();
  mpc_time += end - start;
  if(print_times) {
    std::cout << "local_time: " << local_time << " s\n";
    std::cout << "mpc_time: " << mpc_time << " s\n";
  }
  // }, "SCS Yao protocol (Server)");
}

template<typename K, typename V>
void pir_protocol_scs<K,V>::run_client(
  const pir_protocol_scs<K,V>::key_range input,
  pir_protocol_scs<K,V>::value_range output
) {
  double local_time = 0, mpc_time = 0;
  double start = timestamp(), end;
  size_t input_size = boost::size(input);
  std::map<K, size_t> input_map;
  std::vector<uint8_t> input_bytes(input_size * sizeof(K));
  std::vector<uint8_t> output_keys_bytes(boost::size(output) * sizeof(K));
  std::vector<uint8_t> output_values_bytes(boost::size(output) * sizeof(V));

  // benchmark([&]{
  auto it = boost::begin(input);
  for(size_t i = 0; i < input_size; i++) {
    input_map.emplace(*(it++), i);
  }
  serialize_le(input_bytes.begin(),
    boost::begin(boost::adaptors::keys(input_map)), input_size);
  // }, "Copying inputs (client)");

  // benchmark([&]{
  pir_scs_oblivc_args args = {
    .key_type_size = sizeof(K),
    .value_type_size = sizeof(V),
    .num_elements = input_size,
    .input_keys = input_bytes.data(),
    .input_values = nullptr,
    .input_defaults = nullptr,
    .result_keys = output_keys_bytes.data(),
    .result_values = output_values_bytes.data()
  };

  end = timestamp();
  local_time += end - start;
  start = end;

  // run yao's protocol using Obliv-C
  ProtocolDesc pd;
  chan.flush();
  if(chan.connect_to_oblivc(pd, 10 /* ms between attempts */) == -1) {
    BOOST_THROW_EXCEPTION(std::runtime_error("run_client: connection failed"));
  }
  setCurrentParty(&pd, 2);
  execYaoProtocol(&pd, pir_scs_oblivc, &args);
  cleanupProtocol(&pd);
  // }, "SCS Yao protocol (Client)");

  end = timestamp();
  mpc_time += end - start;
  start = end;

  // benchmark([&]{
  // reorder outputs to match original order of the inputs
  std::vector<V> output_values(input_size);
  for(size_t i = 0; i < input_size; i++) {
    K key; V value;
    deserialize_le(&key, &output_keys_bytes[i * sizeof(K)], 1);
    deserialize_le(&value, &output_values_bytes[i * sizeof(V)], 1);
    output_values[input_map[key]] = value;
  }
  boost::copy(output_values, boost::begin(output));

  end = timestamp();
  local_time += end - start;
  if(print_times) {
    std::cout << "local_time: " << local_time << " s\n";
    std::cout << "mpc_time: " << mpc_time << " s\n";
  }
  // }, "Copying results");
}
