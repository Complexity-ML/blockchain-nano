#!/bin/bash
set -u
mkdir /build/capella-pruning-restore-test-20261004/fixture-scratch
export TMPDIR=/build/capella-pruning-restore-test-20261004/fixture-scratch
/build/capella-pruning-restore-test-20261004/nano-capella-pruning-restore-tests --historical-pruning-restore > /report/attempt2-test.log 2>&1
status=$?
printf "%s\n" "$status" > /report/attempt2-test-exit.txt
sha256sum /build/capella-pruning-restore-test-20261004/nano-capella-pruning-restore-tests > /report/program-after-retry.txt
exit "$status"
