#!/usr/bin/env bash
set -euo pipefail

host="nook@81.26.187.140"
key="/home/blokhtin/.ssh/ssh-key-1786910193185"
remote_root="/home/nook/SCE/texel"
evidence_dir="/tmp/simple-texel-evidence"

cmake --build build-host -j 22
ctest --test-dir build-host -E '^BestMove\.WinAtChess$' --output-on-failure
mkdir -p "$evidence_dir"
scp -i "$key" -o StrictHostKeyChecking=no \
  "$host:$remote_root/positions-v2.summary.json" \
  "$evidence_dir/positions-v2.summary.json"
scp -i "$key" -o StrictHostKeyChecking=no \
  "$host:$remote_root/texel-tactical-0.00001.json" \
  "$evidence_dir/texel-state.json"
scp -i "$key" -o StrictHostKeyChecking=no \
  "$host:$remote_root/logs/texel-pair-knight-passed4.log" \
  "$evidence_dir/pair-256.log"
scp -i "$key" -o StrictHostKeyChecking=no \
  "$host:$remote_root/logs/texel-final-knight-passed4.log" \
  "$evidence_dir/pair-768.log"
python3 Tests/texel_evidence.py \
  --dataset-summary "$evidence_dir/positions-v2.summary.json" \
  --texel-state "$evidence_dir/texel-state.json" \
  "$evidence_dir/pair-256.log" "$evidence_dir/pair-768.log"
