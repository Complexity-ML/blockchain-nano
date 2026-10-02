set -euo pipefail
build=/build/coherent-80373152-amd64-20261002
scratch=/scratch-volume/reindex-profile-80373152-20261002
mkdir "$scratch"
export TMPDIR="$scratch"
sha256sum -c /reports/qualified-program.sha256 > /reports/qualified-program-check.txt
sha256sum -c /reports/qualified-inputs.sha256 > /reports/qualified-input-check.txt
sha256sum "$build/nano-capella-transition-tests" > /reports/program-before.sha256
find "$build" -type f \( -name '*.a' -o -name '*.so' \) -print0 | sort -z | xargs -0 sha256sum > /reports/libs-before.sha256
set +e
timeout 360 "$build/nano-capella-transition-tests" --deposit-reorg-activation > /reports/runtime.log 2>&1
result=$?
set -e
printf '%s\n' "$result" > /reports/diagnostic-runtime-exit.txt
sha256sum "$build/nano-capella-transition-tests" > /reports/program-after.sha256
find "$build" -type f \( -name '*.a' -o -name '*.so' \) -print0 | sort -z | xargs -0 sha256sum > /reports/libs-after.sha256
cmp /reports/program-before.sha256 /reports/program-after.sha256
cmp /reports/libs-before.sha256 /reports/libs-after.sha256
printf '0\n' > /reports/manifest-exit.txt
