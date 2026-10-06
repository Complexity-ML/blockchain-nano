#!/bin/bash
set -eu
out=/build/dispatch-recovery-$1-1f521b7b-20261006-r2
cp "$out/transport-compile-command.txt" "$out/transport-first-compile-command.txt"
cp "$out/transport-run.log" "$out/transport-first-run.log"
eval "$(cat "$out/transport-compile-command.txt")" > "$out/transport-compile.log" 2>&1
(cd /build/coherent-efbeb9bc-$1;eval "$(cat "$out/transport-link-command.txt")") > "$out/transport-link.log" 2>&1
"$out/transport" > "$out/transport-run.log" 2>&1
sha256sum /diagnostic/tests/test_dispatch_transport_recovery.cpp "$out/transport" > "$out/transport-final.sha256"
cat "$out/transport-run.log"
