#!/usr/bin/env bash
set -euo pipefail
while sudo docker inspect nano-coherent-0275b1af-amd64-build >/dev/null 2>&1; do sleep 15; done
for attempt in $(seq 1 12); do test -f /home/ubuntu/nano-coherent-0275b1af-amd64-build-reports-20261002/source-compare-exit.txt && break; sleep 5; done
# run.sh verifies terminal build/source/integrity before any CTest starts.
exec bash /tmp/nano-coherent-0275b1af-test-control-20261002/amd64.sh
