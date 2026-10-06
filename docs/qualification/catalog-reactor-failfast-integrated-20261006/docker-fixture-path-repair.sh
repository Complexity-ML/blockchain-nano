#!/bin/bash
set -eu
arch=$1
out=/build/catalog-reactor-failfast-$arch-20261006
trap 'echo $? > "$out/fixture-path-repair-exit.txt"' EXIT
for mode in reactor-failfast generation-reservation capture-control worker-control worker-http; do
 envvar=NANO_TEST_CATALOG_REACTOR_FAILFAST;arg=--beacon-reads
 [ "$mode" != generation-reservation ] || envvar=NANO_TEST_CATALOG_GENERATION_RACE
 [ "$mode" != capture-control ] || envvar=NANO_TEST_CATALOG_CAPTURE_UNLOCKED
 [ "$mode" != worker-control ] || envvar=NANO_TEST_CATALOG_WORKER_FAILFAST
 if [ "$mode" = worker-http ];then envvar=NANO_TEST_CATALOG_WORKER_FAILFAST;arg=--queued-bls-rpc;fi
 env "$envvar=1" "$out/candidate" "$arg" > "$out/test-$mode.log" 2>&1
 echo 0 > "$out/test-$mode-exit.txt"
 echo "passed $mode"
done
sha256sum -c "$out/parent-before.sha256" > "$out/parent-unchanged.log"
sha256sum "$out/candidate" "$out/libnano_core.a" > "$out/programs.sha256"
mkdir /work/$arch
cp "$out"/*.log "$out"/*.txt "$out"/*.d "$out"/*.sha256 /work/$arch/
