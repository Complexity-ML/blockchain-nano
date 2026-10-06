#!/bin/bash
set -eu
arch=$1
src=parent-cache-$arch-20261006
cd /build
for receipt in controller-exit.txt regression-controller-exit.txt parent-regression-controller-exit.txt runtime-controller-exit.txt;do test "$(cat "$src/$receipt")" = 0;done
find "$src" -type f \( -name '*.log' -o -name '*.txt' -o -name '*.sha256' -o -name '*.d' -o -name '*.exit' \) -print0 | tar -czf "/proof/$arch-evidence.tar.gz" --null -T -
