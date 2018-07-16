#!/bin/bash

config=$(mktemp)
for n in 500 5000 50000; do
  for m_frac in {1..10}; do
    m=$(((n * m_frac) / 10))
    for type in poly scs fss_cprg; do
      echo "num_elements_server = $n" >> $config
      echo "num_elements_client = $m" >> $config
      echo "pir_type= $type" >> $config
    done
  done
done
cat >> $config << EOF
statistical_security = 40
EOF

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
$DIR/../../bin/benchmark/pir -c $config "$@"
