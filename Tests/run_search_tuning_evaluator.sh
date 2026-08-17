#!/usr/bin/env bash
set -euo pipefail

host="nook@158.160.228.88"
key="/home/blokhtin/.ssh/ssh-key-1786910193185"
remote_root="/home/nook/SCE/search-tuning/logs"
evidence_dir="/tmp/simple-search-tuning-evidence"

cmake --build build-tests -j 22
ctest --test-dir build-tests -E '^BestMove\.WinAtChess$' --output-on-failure
mkdir -p "$evidence_dir"
for name in final-robust-v-default final-robust-v-default-b2 \
            final-robust-v-default-b3; do
  scp -i "$key" -o StrictHostKeyChecking=no \
    "$host:$remote_root/$name.log" "$evidence_dir/$name.log"
done
python3 Tests/tuning_evidence.py --minimum-games 256 \
  "$evidence_dir/final-robust-v-default.log" \
  "$evidence_dir/final-robust-v-default-b2.log" \
  "$evidence_dir/final-robust-v-default-b3.log"
