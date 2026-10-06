#!/bin/bash
set -eu
arch=$1
out=/build/reference-preflight-$arch-20261006
trap 'echo $? > /probe/controller-exit.txt' EXIT
sha256sum -c /original-probe/program-before.sha256 > /probe/programs-reused-check.log
sha256sum -c /original-probe/parent-before.sha256 > /probe/parent-reused-check.log
cp /original-probe/program-before.sha256 /probe/program-before.sha256
cp /original-probe/parent-before.sha256 /probe/parent-before.sha256
run_case(){ local name=$1;shift;set +e;"$@" > /probe/test-$name.log 2>&1;local result=$?;echo "$result" > /probe/test-$name-exit.txt;set -e;echo "finished $name exit=$result";return "$result";}
run_case writer-preflight env NANO_TEST_REFERENCE_PREFLIGHT=1 NANO_TEST_REFERENCE_VALIDATION_MEMO=1 NANO_TEST_COMPACT_REFERENCE_RESTORE=1 NANO_TEST_COMPACT_REFERENCE_WRITER=1 NANO_TEST_COLD_RECORD_CACHE=1 NANO_TEST_FRAGMENT_CHECKPOINT=1 "$out/test" --prepared-block
run_case cold-faults env NANO_TEST_FRAGMENT_CHECKPOINT=1 NANO_TEST_COLD_REACTOR=1 NANO_TEST_RUNTIME_REFERENCE_MEMO=1 NANO_TEST_COLD_MEMO_SYNC=1 NANO_TEST_COLD_MEMO_SIGKILL=1 NANO_TEST_DUPLICATE_GUARDS=1 "$out/test" --prepared-block
run_case cold-automatic env NANO_TEST_RUNTIME_REFERENCE_MEMO=1 NANO_TEST_COLD_RUNTIME=1 "$out/test" --automatic-recovery
run_case cold-bls env NANO_TEST_RUNTIME_REFERENCE_MEMO=1 NANO_TEST_COLD_RUNTIME=1 "$out/test" --queued-bls-rpc
sha256sum -c /probe/parent-before.sha256 > /probe/parent-unchanged.log
sha256sum -c /probe/program-before.sha256 > /probe/program-unchanged.log
