#!/bin/bash
set -eu
arch=$1
trap 'echo $? > /probe/controller-exit.txt' EXIT
sha256sum -c /qualified/programs-before.sha256 > /probe/programs-before.log
sha256sum -c /qualified/parent-before.sha256 > /probe/parent-before.log
set +e
NANO_TEST_COMPACT_REFERENCE_RESTORE=1 NANO_TEST_COMPACT_REFERENCE_WRITER=1 NANO_TEST_FRAGMENT_CHECKPOINT=1 NANO_TEST_COLD_RECORD_CACHE=1 /build/cold-header-links-v2-$arch-20261006/test --prepared-block > /probe/test-reference-links.log 2>&1
result=$?
echo "$result" > /probe/test-reference-links-exit.txt
set -e
[ "$result" -eq 0 ]
grep -Fq '"singleRecordCacheSupported":true' /probe/test-reference-links.log
sha256sum -c /qualified/programs-before.sha256 > /probe/programs-unchanged.log
sha256sum -c /qualified/parent-before.sha256 > /probe/parent-unchanged.log
