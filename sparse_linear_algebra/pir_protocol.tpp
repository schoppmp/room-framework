#include <boost/iterator/zip_iterator.hpp>
#include <boost/fusion/adapted/std_pair.hpp>
#include "util/combine_pair.hpp"

// implicit conversion to pair_range since any_range doesn't work as expected
template<typename K, typename V>
void pir_protocol<K, V>::run_server(
  boost::any_range<std::pair<const K, V>, boost::single_pass_traversal_tag,
  std::pair<const K, V>&, boost::use_default> input,
  pir_protocol<K, V>::value_range defaults,
  bool shared_output
) {
  run_server(
    pir_protocol<K, V>::pair_range(boost::begin(input), boost::end(input)),
    defaults, shared_output
  );
}

template<typename K, typename V>
void pir_protocol<K, V>::run_server(
  const pir_protocol<K, V>::key_range input_keys,
  const pir_protocol<K, V>::value_range input_values,
  const pir_protocol<K, V>::value_range defaults,
  bool shared_output
) {
  using pair_iterator = typename pir_protocol<K, V>::pair_range::iterator;
  // convert inner iterator type before calling run_server
  pair_iterator it(boost::begin(combine_pair(input_keys, input_values)));
  run_server(boost::make_iterator_range_n(it, boost::size(input_keys)),
    defaults, shared_output);
}
