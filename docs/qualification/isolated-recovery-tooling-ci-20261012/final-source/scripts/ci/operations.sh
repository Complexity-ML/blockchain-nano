#!/bin/sh
set -eu
mkdir -p ci-reports
node --version > ci-reports/node-version.txt
git rev-parse HEAD > ci-reports/source-revision.txt
# Only self-contained tests: no live Docker/SSH, keys or external receiver.
# Required recovery tests must not disappear silently from a shell glob.
for required in tests/capella/isolated-fixture-domain.test.mjs tests/capella/isolated-bootstrap-plan.test.mjs tests/capella/isolated-runtime-observation.test.mjs tests/capella/pinned-json-input.test.mjs tests/capella/isolated-core-source.test.mjs scripts/ci/check-core.test.mjs; do
 test -f "$required" || { echo "Missing required recovery test: $required" >&2; exit 1; }
done
node --test scripts/operations/*.test.mjs scripts/ci/*.test.mjs tests/multihost/*.test.mjs tests/capella/*.test.mjs tools/bridge-source/*.test.mjs > ci-reports/operations.log 2>&1 || {
 cat ci-reports/operations.log
 exit 1
}
cat ci-reports/operations.log

# Check direct-dependency notice integrity without compiling the runtime.
node scripts/operations/check-notices.mjs > ci-reports/notices.json
