#!/bin/bash

config=$(mktemp)

for m in 1 5 10 50 100 500 1000 5000; do
  for k_A in 1 5 10 50 100 500 1000 5000 10000 50000; do
    echo "inner_dim = $m" >> $config
    echo "nonzeros_server = $k_A" >> $config
  done
done

cat >> $config << EOF
cols_client = 1
rows_server = 150000
multiplication_type = rows_dense
pir_type = basic
nonzeros_client = 1 # doesn't matter

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
