#!/bin/bash
set -eu
arch=$1
out=/build/fragment-runtime-$arch-20261006
mkdir /probe/initial-fixture-path-failure
cp /probe/controller-exit.txt /probe/test-fragment.log /probe/initial-fixture-path-failure/
trap 'echo $? > /probe/test-controller-exit.txt' EXIT
NANO_TEST_FRAGMENT_CHECKPOINT=1 "$out/test" --prepared-block > /probe/test-fragment.log 2>&1
echo 0 > /probe/test-fragment-exit.txt
for mode in owner-witness configured-runtime automatic-recovery;do
 "$out/test" --"$mode" > /probe/test-$mode.log 2>&1
 echo 0 > /probe/test-$mode-exit.txt
 echo "passed $mode"
done
sha256sum -c /probe/parent-before.sha256 > /probe/parent-unchanged.log
sha256sum "$out/test" "$out/libnano_core.a" > /probe/programs.sha256
printf '%s fragment integration qualified\n' "$arch"
