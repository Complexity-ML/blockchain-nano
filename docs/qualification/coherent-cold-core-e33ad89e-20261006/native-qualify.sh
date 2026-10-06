#!/bin/bash
set -eu
root=/private/tmp/nano-coherent-e33ad89e-20261006
out=$root/native-qualification
bin=$root/api-repair/native-v2/test
trap 'echo $? > "$out/controller-exit.txt"' EXIT
shasum -a 256 "$bin" "$root/native-build/nano" "$root/native-build/libnano_core.a" > "$out/programs-before.sha256"
run(){ local name=$1; shift; set +e; "$@" > "$out/$name.log" 2>&1; local rc=$?; echo "$rc" > "$out/$name-exit.txt"; set -e; echo "$name exit=$rc"; return "$rc"; }
run cold-guards env NANO_TEST_RUNTIME_REFERENCE_MEMO=1 NANO_TEST_COLD_REACTOR=1 NANO_TEST_FRAGMENT_CHECKPOINT=1 NANO_TEST_DUPLICATE_GUARDS=1 "$bin" --prepared-block
run cold-automatic env NANO_TEST_RUNTIME_REFERENCE_MEMO=1 NANO_TEST_COLD_RUNTIME=1 "$bin" --automatic-recovery
run cold-bls env NANO_TEST_RUNTIME_REFERENCE_MEMO=1 NANO_TEST_COLD_RUNTIME=1 "$bin" --queued-bls-rpc
shasum -a 256 -c "$out/programs-before.sha256" > "$out/programs-unchanged.log"
