#!/usr/bin/env bash
set -euo pipefail
# Do not contend with the unchanged complete regression on this Docker host.
while docker inspect nano-full-coherent-2d2764de-arm64-20261002-test >/dev/null 2>&1 || docker inspect nano-coherent-80373152-arm64-build >/dev/null 2>&1; do sleep 15; done
for attempt in $(seq 1 12); do test -f /tmp/nano-coherent-80373152-arm64-build-reports-20261002/source-compare-exit.txt && break; sleep 5; done
exec bash /tmp/nano-coherent-80373152-test-control-20261002/arm64.sh
