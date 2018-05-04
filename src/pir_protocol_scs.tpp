#include "utils.h"
extern "C" {
  #include <obliv.h>
  #include "pir_protocol_scs.h"
}
#include <boost/range/adaptor/map.hpp>

template<typename K, typename V>
void pir_protocol_scs<K,V>::run_server(
  const pir_protocol_scs<K,V>::pair_range input,
  const pir_protocol_scs<K,V>::value_range defaults
) {
  // sort inputs
  std::vector<std::pair<K,V>> input_vec;
  std::vector<uint8_t> input_keys_bytes(boost::size(input) * sizeof(K));
  std::vector<uint8_t> input_values_bytes(boost::size(input) * sizeof(V));
  std::vector<uint8_t> input_defaults_bytes(boost::size(input) * sizeof(V));
  // benchmark([&]{
  input_vec = std::vector<std::pair<K,V>>(boost::begin(input), boost::end(input));
  boost::sort(input_vec);
  // serialize for obliv-c
  serialize_le(input_keys_bytes.begin(), boost::begin(boost::adaptors::keys(input_vec)), boost::size(input));
  serialize_le(input_values_bytes.begin(), boost::begin(boost::adaptors::values(input_vec)), boost::size(input));
  serialize_le(input_defaults_bytes.begin(), boost::begin(defaults), boost::size(defaults));
  // }, "Copying inputs (server)");

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
  // run yao's protocol using Obliv-C
  ProtocolDesc pd;
  chan.flush();
  if(chan.connect_to_oblivc(pd, 10 /* ms between attempts */) == -1) {
    BOOST_THROW_EXCEPTION(std::runtime_error("run_server: connection failed"));
  }
  setCurrentParty(&pd, 1);
  execYaoProtocol(&pd, pir_scs_oblivc, &args);
  cleanupProtocol(&pd);
  // }, "SCS Yao protocol (Server)");
}

template<typename K, typename V>
void pir_protocol_scs<K,V>::run_client(
  const pir_protocol_scs<K,V>::key_range input,
  pir_protocol_scs<K,V>::value_range output
) {
    std::vector<K> input_vec;
    std::vector<uint8_t> input_bytes(boost::size(input) * sizeof(K));
    std::vector<uint8_t> output_keys(boost::size(output) * sizeof(K));
    std::vector<uint8_t> output_values(boost::size(output) * sizeof(V));
    // benchmark([&]{
    input_vec = std::vector<K>(boost::begin(input), boost::end(input));
    boost::sort(input_vec);
    serialize_le(input_bytes.begin(), input_vec.begin(), input_vec.size());
    // }, "Copying inputs (client)");

    // benchmark([&]{
    pir_scs_oblivc_args args = {
      .key_type_size = sizeof(K),
      .value_type_size = sizeof(V),
      .num_elements = boost::size(input),
      .input_keys = input_bytes.data(),
      .input_values = nullptr,
      .input_defaults = nullptr,
      .result_keys = output_keys.data(),
      .result_values = output_values.data()
    };
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

    benchmark([&]{
      // use a map to get result values back into the order of the inputs
      // TODO: this is horribly slow! speed it up by saving pointers into the
      // output range next to input keys before sorting.
      std::map<K,V> result_map;
      for(size_t i = 0; i < boost::size(input); i++) {
        K key; V value;
        deserialize_le(&key, &output_keys[i * sizeof(K)], 1);
        deserialize_le(&value, &output_values[i * sizeof(V)], 1);
        result_map.emplace(key, value);
      }
      auto it_out = boost::begin(output);
      for(auto it_in = boost::begin(input); it_in != boost::end(input); it_in++,it_out++) {
        *it_out = result_map[*it_in];
      }
    }, "Copying results");
}
