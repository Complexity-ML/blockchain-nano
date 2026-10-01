#!/bin/sh
set -eu
mkdir -p ci-reports
node --version > ci-reports/node-version.txt
git rev-parse HEAD > ci-reports/source-revision.txt
# Only self-contained tests: no live Docker/SSH, keys or external receiver.
node --test scripts/operations/*.test.mjs scripts/ci/*.test.mjs tests/multihost/*.test.mjs > ci-reports/operations.log 2>&1 || {
 cat ci-reports/operations.log
 exit 1
}
cat ci-reports/operations.log
