#!/bin/bash
set -eu
arch=$1
out=/build/cold-runtime-$arch-20261006
trap 'echo $? > /probe/controller-exit.txt' EXIT
sha256sum -c /qualified/programs-before.sha256 > /probe/programs-before-verified.log
sha256sum -c /qualified/parent-before.sha256 > /probe/parent-before-verified.log
sha256sum /work/tests/fixtures/ethereum-blocks/for_prague--prague--eip6110_deposits--deposits--deposit.json > /probe/fixture.sha256
set +e
"$out/test" --pruning-automatic > /probe/test-warm-pruning.log 2>&1
result=$?
echo "$result" > /probe/test-warm-pruning-exit.txt
set -e
[ "$result" -eq 0 ]
sha256sum -c /qualified/programs-before.sha256 > /probe/programs-unchanged.log
sha256sum -c /qualified/parent-before.sha256 > /probe/parent-unchanged.log
