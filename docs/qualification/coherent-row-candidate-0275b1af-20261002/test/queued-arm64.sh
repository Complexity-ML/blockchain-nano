#!/usr/bin/env bash
set -euo pipefail
# Do not contend with the unchanged complete regression on this Docker host.
while docker inspect nano-full-coherent-2d2764de-arm64-20261002-test >/dev/null 2>&1 || docker inspect nano-coherent-0275b1af-arm64-build >/dev/null 2>&1; do sleep 15; done
for attempt in $(seq 1 12); do test -f /tmp/nano-coherent-0275b1af-arm64-build-reports-20261002/source-compare-exit.txt && break; sleep 5; done
exec bash /tmp/nano-coherent-0275b1af-test-control-20261002/arm64.sh
