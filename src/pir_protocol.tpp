#include <boost/iterator/zip_iterator.hpp>
#include <boost/range/iterator_range_core.hpp>
#include <boost/fusion/adapted/std_pair.hpp>
#include <boost/range/combine.hpp>

// templates for calling the explicit any_iterator constructor

// template<typename K, typename V>
// template<typename Iterator1, typename Iterator2>
// void pir_protocol<K, V>::run_server(const Iterator1 input_first,
//   size_t input_length,
//   const Iterator2 default_first, size_t default_length
// ) {
//   PairIterator input_first2(input_first);
//   ValueIterator default_first2(default_first);
//   run_server(input_first2, input_length, default_first2, default_length);
// }
//
// template<typename K, typename V>
// template<typename Iterator1, typename Iterator2>
// void pir_protocol<K, V>::run_client(const Iterator1 input_first,
//   const Iterator2 output_first, size_t length
// ) {
//   KeyIterator input_first2(input_first);
//   ValueIterator output_first2(output_first);
//   run_client(input_first2, output_first2, length);
// }
//
// template<typename K, typename V>
// template<typename Iterator1, typename Iterator2>
// void pir_protocol<K, V>::run_server(const Iterator1 input_keys_it,
//   const Iterator2 input_values_it, size_t input_length,
//   const Iterator2 default_it, size_t default_length
// ) {
//   KeyIterator input_keys_it2(input_keys_it);
//   ValueIterator input_values_it2(input_values_it);
//   ValueIterator default_it2(default_it);
//   run_server(input_keys_it2, input_values_it2, input_length, default_it2,
//     default_length);
// }
//
//
// // implementations of virtual methods that don't need to be implemented by
// // subclasses
//
// template<typename K, typename V>
// void pir_protocol<K, V>::run_server(
//   const pir_protocol<K, V>::KeyIterator input_keys_it,
//   const pir_protocol<K, V>::ValueIterator input_values_it, size_t input_length,
//   const pir_protocol<K, V>::ValueIterator default_it, size_t default_length
// ) {
//   auto pair_it = boost::make_zip_iterator(std::make_pair(input_keys_it,
//     input_values_it));
//   run_server(pair_it, input_length, default_it,
//     default_length);
// }

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
  auto end_pair = std::make_pair(boost::end(input_keys), boost::end(input_values));
  pair_iterator_type begin_iterator(boost::make_zip_iterator(begin_pair));
  pair_iterator_type end_iterator(boost::make_zip_iterator(end_pair));
  run_server(boost::make_iterator_range(begin_iterator, end_iterator), defaults);
  // run_server(boost::range::combined_range<decltype(begin_pair)>(begin_pair, end_pair), defaults);
}
