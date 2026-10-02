#!/usr/bin/env bash
set -euo pipefail
reports=/tmp/nano-coherent-0275b1af-native-build-reports-20261002
for attempt in $(seq 1 240); do
  if test -f "$reports/controller-exit.txt"; then
    test "$(cat "$reports/controller-exit.txt")" = 0
    bash /tmp/nano-coherent-0275b1af-native-test-control-20261002/run.sh
    exec python3 /tmp/nano-coherent-0275b1af-native-offline-control-20261002/run.py
  fi
  if test -f "$reports/build-exit.txt" && test "$(cat "$reports/build-exit.txt")" != 0; then exit 1; fi
  sleep 15
done
exit 124
