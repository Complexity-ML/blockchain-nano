#!/bin/bash
set -eu
arch=$1
out=/build/coherent-e33ad89e-$arch
trap 'echo $? > /probe/controller-exit.txt' EXIT
sha256sum -c /build-proof/programs.sha256 > /probe/programs-before.log
set +e
ctest --test-dir "$out" --output-on-failure -R '^consensus_capella_cold_(reference_writer|memo_sigkill_recovery|runtime_recovery|runtime_bls)$' > /probe/ctest.log 2>&1
rc=$?
echo "$rc" > /probe/ctest-exit.txt
set -e
cp "$out/Testing/Temporary/LastTest.log" /probe/LastTest.log
if [ -f "$out/Testing/Temporary/LastTestsFailed.log" ]; then cp "$out/Testing/Temporary/LastTestsFailed.log" /probe/LastTestsFailed.log; fi
sha256sum -c /build-proof/programs.sha256 > /probe/programs-after.log
exit "$rc"
