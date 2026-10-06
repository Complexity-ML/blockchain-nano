#!/bin/bash
set -eu
root=/private/tmp/nano-coherent-e33ad89e-20261006
out=$root/native-bls-local-bind
trap 'echo $? > "$out/controller-exit.txt"' EXIT
shasum -a 256 -c "$root/native-qualification/programs-before.sha256" > "$out/programs-before.log"
set +e
env NANO_TEST_RUNTIME_REFERENCE_MEMO=1 NANO_TEST_COLD_RUNTIME=1 "$root/api-repair/native-v2/test" --queued-bls-rpc > "$out/cold-bls.log" 2>&1
rc=$?; echo "$rc" > "$out/cold-bls-exit.txt"; set -e
shasum -a 256 -c "$root/native-qualification/programs-before.sha256" > "$out/programs-after.log"
exit "$rc"
