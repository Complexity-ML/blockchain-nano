#!/bin/bash
set -eu
bash /reports/network-build.sh
sha256sum /prior/nano-altair-p2p-diagnostic /build/network/nano-p2p /build/network/nano-altair-probe /reports/nano-altair-probe-current > /reports/programs-before.sha256
set +e
timeout -s KILL 120 /prior/nano-altair-p2p-diagnostic /build/network/nano-p2p /reports/nano-altair-probe-current > /reports/test.stdout 2> /reports/test.stderr
code=$?
set -e
printf '%s\n' "$code" > /reports/test-exit.txt
sha256sum /prior/nano-altair-p2p-diagnostic /build/network/nano-p2p /build/network/nano-altair-probe /reports/nano-altair-probe-current > /reports/programs-after.sha256
cmp /reports/programs-before.sha256 /reports/programs-after.sha256
exit "$code"
