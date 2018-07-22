#!/bin/bash

config=$(mktemp)

for l in 1 10 100 1000 10000; do
  for k_A in 1 10 100 1000 10000 100000; do
    for k_B in 1 10 100 1000 10000 100000; do
      for pir_type in scs poly; do
        echo "rows_server = $l" >> $config
        echo "nonzero_cols_server = $k_A" >> $config
        echo "nonzero_rows_client = $k_A" >> $config
        echo "pir_type = $pir_type" >> $config
      done
    done
  done
done

cat >> $config << EOF
cols_client = 1
inner_dim = 150000
statistical_security = 40

# for local testing only
[server]
host=localhost
port=12347

[server]
host=localhost
port=12357
EOF

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
$DIR/../../bin/benchmark/matrix_multiplication -c "$config" "$@"
