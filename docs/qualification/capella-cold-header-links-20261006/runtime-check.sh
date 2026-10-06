#!/bin/bash
set -eu
arch=$1
trap 'echo $? > /probe/controller-exit.txt' EXIT
sha256sum -c /qualified/programs-before.sha256 > /probe/programs-before.log
sha256sum -c /qualified/parent-before.sha256 > /probe/parent-before.log
set +e
NANO_TEST_COLD_RUNTIME=1 /build/cold-header-links-v2-$arch-20261006/test --automatic-recovery > /probe/test-cold-runtime.log 2>&1
result=$?
echo "$result" > /probe/test-cold-runtime-exit.txt
set -e
[ "$result" -eq 0 ]
grep -Fq '"coldFinalizedPruning":true' /probe/test-cold-runtime.log
sha256sum -c /qualified/programs-before.sha256 > /probe/programs-unchanged.log
sha256sum -c /qualified/parent-before.sha256 > /probe/parent-unchanged.log
