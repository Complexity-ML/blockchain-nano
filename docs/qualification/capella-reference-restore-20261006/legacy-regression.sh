#!/bin/bash
set -eu
arch=$1
out=/build/reference-restore-$arch-20261006
trap 'echo $? > /probe/legacy-regression-controller-exit.txt' EXIT
for mode in configured-runtime automatic-recovery;do
 "$out/test" --"$mode" > /probe/test-$mode.log 2>&1
 echo 0 > /probe/test-$mode-exit.txt
done
sha256sum -c /probe/programs-before.sha256 > /probe/legacy-programs-unchanged.log
