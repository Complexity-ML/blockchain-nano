set -euo pipefail
build=/build/published-bytes-fa390c3b-20261002
mkdir /build/qualification-capella-reindex-profile-20261002
export TMPDIR=/build/qualification-capella-reindex-profile-20261002
printf '%s  %s\n' a125a741e46edff641f2aa8ce03b5984ade96c69c65735752f6b19379dc0baa9 "$build/nano-capella-transition-tests" | sha256sum -c -
sha256sum "$build/nano-capella-transition-tests" > /reports/program-before.sha256
find "$build" -type f \( -name '*.a' -o -name '*.so' \) -print0 | sort -z | xargs -0 sha256sum > /reports/libs-before.sha256
set +e
timeout 360 "$build/nano-capella-transition-tests" --deposit-reorg-activation > /reports/runtime.log 2>&1
result=$?;set -e
printf '%s\n' "$result" > /reports/diagnostic-runtime-exit.txt
sha256sum "$build/nano-capella-transition-tests" > /reports/program-after.sha256
find "$build" -type f \( -name '*.a' -o -name '*.so' \) -print0 | sort -z | xargs -0 sha256sum > /reports/libs-after.sha256
cmp /reports/program-before.sha256 /reports/program-after.sha256
cmp /reports/libs-before.sha256 /reports/libs-after.sha256
printf '0\n' > /reports/manifest-exit.txt
