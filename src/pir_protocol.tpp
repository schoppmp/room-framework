#include <boost/iterator/zip_iterator.hpp>
#include <boost/fusion/adapted/std_pair.hpp>

// implicit conversion to pair_range since any_range doesn't work as expected
template<typename K, typename V>
void pir_protocol<K, V>::run_server(
  boost::any_range<std::pair<const K, V>, boost::single_pass_traversal_tag,
  std::pair<const K, V>&, boost::use_default> input, pir_protocol<K, V>::value_range defaults
) {
  run_server(
    pir_protocol<K, V>::pair_range(boost::begin(input), boost::end(input)),
    defaults
  );
}

template<typename K, typename V>
void pir_protocol<K, V>::run_server(
  const pir_protocol<K, V>::key_range input_keys,
  const pir_protocol<K, V>::value_range input_values,
  const pir_protocol<K, V>::value_range defaults
) {
  using pair_iterator_type = typename pir_protocol<K, V>::pair_range::iterator;
  auto begin_pair = std::make_pair(boost::begin(input_keys), boost::begin(input_values));
  pair_iterator_type begin_iterator(boost::make_zip_iterator(begin_pair));
  run_server(boost::make_iterator_range_n(begin_iterator, boost::size(input_keys)), defaults);
}
