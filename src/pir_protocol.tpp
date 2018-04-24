
template<typename K, typename V>
std::vector<V> pir_protocol<K, V>::run_server(const std::map<K,V>& server_in) {
  std::vector<V> ret;
  run_server(server_in, ret);
  return ret;
}

template<typename K, typename V>
std::vector<V> pir_protocol<K, V>::run_client(const std::vector<K>& client_in){
  std::vector<V> ret;
  run_client(client_in, ret);
  return ret;
}
