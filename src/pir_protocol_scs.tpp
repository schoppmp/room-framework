#include "serialize.h"
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
  std::vector<std::pair<K,V>> input_vec(boost::begin(input), boost::end(input));
  boost::sort(input_vec);
  // serialize for obliv-c
  std::vector<uint8_t> input_keys_bytes(boost::size(input) * sizeof(K));
  std::vector<uint8_t> input_values_bytes(boost::size(input) * sizeof(V));
  std::vector<uint8_t> input_defaults_bytes(boost::size(input) * sizeof(V));
  serialize_le(input_keys_bytes.begin(), boost::begin(boost::adaptors::keys(input_vec)), boost::size(input));
  serialize_le(input_values_bytes.begin(), boost::begin(boost::adaptors::values(input_vec)), boost::size(input));
  serialize_le(input_defaults_bytes.begin(), boost::begin(defaults), boost::size(defaults));

  pir_scs_oblivc_args args = {
    .key_type_size = sizeof(K),
    .value_type_size = sizeof(V),
    .num_elements = input_vec.size(),
    .input_keys = input_keys_bytes.data(),
    .input_values = input_values_bytes.data(),
    .input_defaults = input_defaults_bytes.data(),
    .result = nullptr
  };
  // run yao's protocol using Obliv-C
  ProtocolDesc pd;
  chan.flush();
  if(chan.connect_to_oblivc(pd) == -1) {
    BOOST_THROW_EXCEPTION(std::runtime_error("run_server: connection failed"));
  }
  setCurrentParty(&pd, 1);
  execYaoProtocol(&pd, pir_poly_oblivc, &args);
  cleanupProtocol(&pd);
}

template<typename K, typename V>
void pir_protocol_scs<K,V>::run_client(
  const pir_protocol_scs<K,V>::key_range input,
  const pir_protocol_scs<K,V>::value_range output
) {
  std::vector<uint8_t> input_bytes(boost::size(input) * sizeof(K));
  std::vector<uint8_t> output_bytes(boost::size(output) * sizeof(V));
  serialize_le(input_bytes.begin(), boost::begin(input), boost::size(input));
  pir_scs_oblivc_args args = {
    .key_type_size = sizeof(K),
    .value_type_size = sizeof(V),
    .num_elements = boost::size(input),
    .input_keys = input_bytes.data(),
    .input_values = nullptr,
    .input_defaults = nullptr,
    .result = output_bytes.data()
  };
  // run yao's protocol using Obliv-C
  ProtocolDesc pd;
  chan.flush();
  if(chan.connect_to_oblivc(pd) == -1) {
    BOOST_THROW_EXCEPTION(std::runtime_error("run_client: connection failed"));
  }
  setCurrentParty(&pd, 2);
  execYaoProtocol(&pd, pir_poly_oblivc, &args);
  cleanupProtocol(&pd);
  deserialize_le(boost::begin(output), output_bytes.data(), boost::size(output));
}
