#!/bin/bash

# 'Movies': (95626, 136),
# 'Newsgroups': (101631, 98),
# 'Languages, ngrams=1': (1033, 43),
# 'Languages, ngrams=2': (9915, 231),

config=$(mktemp)
for dataset in 95626,136 101631,98 1033,43 9915,231; do
  d=$(echo $dataset | cut -d, -f1)
  s=$(echo $dataset | cut -d, -f2)
  for type in basic poly scs; do
    if [ $type == "basic" ]; then
      echo "num_elements_server = 150000" >> $config
    else
      echo "num_elements_server = $d" >> $config
    fi
    echo "num_elements_client = $s" >> $config
    echo "pir_type= $type" >> $config
  done
done
cat >> $config << EOF
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
$DIR/../../bin/benchmark/pir -c $config "$@"
