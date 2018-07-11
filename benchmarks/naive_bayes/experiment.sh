#!/bin/bash

# 'Movies': (124247.,0.001),
# 'Newsgroups': (101531.,0.001),
# 'Languages, ngrams=1': (1017.,0.029),
# 'Languages, ngrams=5': (272796.,0.0052),

config=$(mktemp)
for dataset in 1124247,124 101531,101 1017,29 272796,1418; do
  d=$(echo $dataset | cut -d, -f1)
  s=$(echo $dataset | cut -d, -f2)
  for type in poly scs fss_cprg; do
    echo "num_elements_server = $d" >> $config
    echo "num_elements_client = $s" >> $config
    echo "pir_type= $type" >> $config
  done
done
cat >> $config << EOF
statistical_security = 40
EOF

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
$DIR/../../bin/benchmark/pir -c $config "$@"
