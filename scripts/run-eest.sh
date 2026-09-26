#!/bin/sh
# Official EEST harness; executes Nano's C++ runners, never another EVM.
set -eu
nano_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
eest_root=${NANO_EEST_ROOT:-"$nano_root/.deps/execution-spec-tests"}
nano_build=${NANO_EEST_BUILD:-"$nano_root/build-release"}
if [ ! -x "$eest_root/.venv/bin/consume" ]; then
 echo "Install the pinned EEST checkout and run uv sync --frozen; see docs/EEST.md" >&2
 exit 2
fi
for runner in nano-ethereum-state-runner nano-ethereum-block-runner; do
 test -x "$nano_build/$runner" || { echo "Missing $nano_build/$runner" >&2; exit 2; }
done
export PYTHONPATH="$nano_root/tests/eest${PYTHONPATH:+:$PYTHONPATH}"
exec "$eest_root/.venv/bin/consume" direct --bin "$nano_build/nano-ethereum-state-runner" -p nano_consumer "$@"
