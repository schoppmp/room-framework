

template<typename K, typename V>
template<typename Iterator1, typename Iterator2>
void pir_protocol<K, V>::run_server(const Iterator1 input_first, size_t input_length,
  const Iterator2 default_first, size_t default_length)
{
    PairIterator input_first2(input_first);
    ValueIterator default_first2(default_first);
    run_server(input_first2, input_length, default_first2, default_length);
}


template<typename K, typename V>
template<typename Iterator1, typename Iterator2>
void pir_protocol<K, V>::run_client(const Iterator1 input_first,
  const Iterator2 output_first, size_t length)
{
    KeyIterator input_first2(input_first);
    ValueIterator output_first2(output_first);
    run_client(input_first2, output_first2, length);
}
