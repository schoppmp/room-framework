#!/bin/bash

config=$(mktemp)

for l in 1 5 10 50 100 500 1000 5000; do
  for k_A in 1 5 10 50 100 500 1000 5000 10000 50000; do
    echo "rows_server = $l" >> $config
    echo "nonzeros_server = $k_A" >> $config
  done
done

cat >> $config << EOF
cols_client = 1
inner_dim = 150000
multiplication_type = cols_dense
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
