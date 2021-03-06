#include "absl/strings/str_cat.h"
#include "boost/range/adaptor/map.hpp"
#include "boost/range/combine.hpp"
#include "boost/range/irange.hpp"
#include "mpc_utils/comm_channel_oblivc_adapter.hpp"
#include "sparse_linear_algebra/util/serialize_le.hpp"
#include "sparse_linear_algebra/util/time.h"
extern "C" {
#include "obliv.h"
#include "sorting_oblivious_map.h"
}

template <typename K, typename V>
void sorting_oblivious_map<K, V>::run_server(
    const sorting_oblivious_map<K, V>::pair_range input,
    const sorting_oblivious_map<K, V>::value_range defaults, bool shared_output,
    mpc_utils::Benchmarker* benchmarker) {
  mpc_utils::Benchmarker::time_point start;
  if (benchmarker != nullptr) {
    start = benchmarker->StartTimer();
  }

  size_t input_size = boost::size(input);
  // sort inputs
  std::vector<std::pair<K, V>> input_vec;
  std::vector<uint8_t> input_keys_bytes(input_size * sizeof(K));
  std::vector<uint8_t> input_values_bytes(input_size * sizeof(V));
  std::vector<uint8_t> input_defaults_bytes(boost::size(defaults) * sizeof(V));

  input_vec =
      std::vector<std::pair<K, V>>(boost::begin(input), boost::end(input));
  boost::sort(input_vec);
  // serialize for obliv-c
  serialize_le(input_keys_bytes.begin(),
               boost::begin(boost::adaptors::keys(input_vec)), input_size);
  serialize_le(input_values_bytes.begin(),
               boost::begin(boost::adaptors::values(input_vec)), input_size);
  serialize_le(input_defaults_bytes.begin(), boost::begin(defaults),
               boost::size(defaults));
  chan.flush();

  pir_scs_oblivc_args args = {.key_type_size = sizeof(K),
                              .value_type_size = sizeof(V),
                              .num_elements = input_vec.size(),
                              .input_keys = input_keys_bytes.data(),
                              .input_values = input_values_bytes.data(),
                              .input_defaults = input_defaults_bytes.data(),
                              .result_keys = nullptr,
                              .result_values = nullptr,
                              .shared_output = shared_output};

  if (benchmarker != nullptr) {
    benchmarker->AddSecondsSinceStart("local_time", start);
    start = benchmarker->StartTimer();
  }

  // run yao's protocol using Obliv-C
  auto status =
      mpc_utils::CommChannelOblivCAdapter::Connect(chan, /*sleep_time=*/10);
  if (!status.ok()) {
    std::string error = absl::StrCat("run_server: connection failed: ",
                                     status.status().message());
    BOOST_THROW_EXCEPTION(std::runtime_error(error));
  }
  ProtocolDesc pd = status.ValueOrDie();
  setCurrentParty(&pd, 1);
  execYaoProtocol(&pd, pir_scs_oblivc, &args);

  if (benchmarker != nullptr && chan.is_measured()) {
    benchmarker->AddAmount("Bytes Sent (Obliv-C)", tcp2PBytesSent(&pd));
  }

  cleanupProtocol(&pd);

  if (benchmarker != nullptr) {
    benchmarker->AddSecondsSinceStart("mpc_time", start);
    start = benchmarker->StartTimer();
  }
}

template <typename K, typename V>
void sorting_oblivious_map<K, V>::run_client(
    const sorting_oblivious_map<K, V>::key_range input,
    sorting_oblivious_map<K, V>::value_range output, bool shared_output,
    mpc_utils::Benchmarker* benchmarker) {
  mpc_utils::Benchmarker::time_point start;
  if (benchmarker != nullptr) {
    start = benchmarker->StartTimer();
  }

  size_t input_size = boost::size(input);
  std::map<K, size_t> input_map;
  std::vector<uint8_t> input_bytes(input_size * sizeof(K));
  std::vector<uint8_t> output_keys_bytes(boost::size(output) * sizeof(K));
  std::vector<uint8_t> output_values_bytes(boost::size(output) * sizeof(V));

  auto it = boost::begin(input);
  for (size_t i = 0; i < input_size; i++) {
    input_map.emplace(*(it++), i);
  }
  serialize_le(input_bytes.begin(),
               boost::begin(boost::adaptors::keys(input_map)), input_size);

  pir_scs_oblivc_args args = {.key_type_size = sizeof(K),
                              .value_type_size = sizeof(V),
                              .num_elements = input_size,
                              .input_keys = input_bytes.data(),
                              .input_values = nullptr,
                              .input_defaults = nullptr,
                              .result_keys = output_keys_bytes.data(),
                              .result_values = output_values_bytes.data(),
                              .shared_output = shared_output};

  if (benchmarker != nullptr) {
    benchmarker->AddSecondsSinceStart("local_time", start);
    start = benchmarker->StartTimer();
  }

  // run yao's protocol using Obliv-C
  auto status =
      mpc_utils::CommChannelOblivCAdapter::Connect(chan, /*sleep_time=*/10);
  if (!status.ok()) {
    std::string error = absl::StrCat("run_server: connection failed: ",
                                     status.status().message());
    BOOST_THROW_EXCEPTION(std::runtime_error(error));
  }
  ProtocolDesc pd = status.ValueOrDie();
  setCurrentParty(&pd, 2);
  execYaoProtocol(&pd, pir_scs_oblivc, &args);

  if (benchmarker != nullptr && chan.is_measured()) {
    benchmarker->AddAmount("Bytes Sent (Obliv-C)", tcp2PBytesSent(&pd));
  }

  cleanupProtocol(&pd);

  if (benchmarker != nullptr) {
    benchmarker->AddSecondsSinceStart("mpc_time", start);
    start = benchmarker->StartTimer();
  }

  // reorder outputs to match original order of the inputs
  std::vector<V> output_values(input_size);
  for (size_t i = 0; i < input_size; i++) {
    K key;
    V value;
    deserialize_le(&key, &output_keys_bytes[i * sizeof(K)], 1);
    deserialize_le(&value, &output_values_bytes[i * sizeof(V)], 1);
    output_values[input_map[key]] = value;
  }
  boost::copy(output_values, boost::begin(output));

  if (benchmarker != nullptr) {
    benchmarker->AddSecondsSinceStart("local_time", start);
  }
}
