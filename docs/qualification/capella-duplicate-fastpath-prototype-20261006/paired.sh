#!/bin/bash
set -eu
arch=$1
base=/build/coherent-efbeb9bc-$arch
out=/build/duplicate-fastpath-$arch-20261006
parent=/build/cold-runtime-$arch-20261006/libnano_core.a
trap 'echo $? > /probe/controller-exit.txt' EXIT
sha256sum -c /qualified/programs-before.sha256 > /probe/candidate-before.log
sha256sum -c /qualified/parent-before.sha256 > /probe/parent-before.log
cmd=$(cat /qualified/link-command.txt)
cmd=${cmd//-o $out\/test /-o $out\/test-reference }
cmd=${cmd//$out\/libnano_core.a/$parent}
printf '%s\n' "$cmd" > /probe/reference-link-command.txt
(cd "$base";eval "$cmd") > /probe/reference-link.log 2>&1
sha256sum "$out/test-reference" > /probe/reference-before.sha256
for pair in 1 2 3;do
 for mode in reference candidate;do
  binary=$out/test;[ "$mode" != reference ] || binary=$out/test-reference
  env NANO_TEST_COLD_REACTOR=1 NANO_TEST_FRAGMENT_CHECKPOINT=1 NANO_TEST_DUPLICATE_BENCH=1 "$binary" --prepared-block > /probe/pair-$pair-$mode.log 2>&1
  echo 0 > /probe/pair-$pair-$mode-exit.txt
 done
done
sha256sum -c /qualified/programs-before.sha256 > /probe/candidate-unchanged.log
sha256sum -c /qualified/parent-before.sha256 > /probe/parent-unchanged.log
sha256sum -c /probe/reference-before.sha256 > /probe/reference-unchanged.log
