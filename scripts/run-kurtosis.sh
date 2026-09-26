#!/bin/sh
# Local devnet only: never use these public fixture keys for real assets.
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
kurtosis=${KURTOSIS_BIN:-"$root/.deps/kurtosis/kurtosis"}
exporter=${NANO_KURTOSIS_EXPORTER:-"$root/build-release/nano-bellatrix-transition-tests"}
image=${NANO_KURTOSIS_IMAGE:-blockchain-nano:local}
enclave=${NANO_KURTOSIS_ENCLAVE:-nano-beacon-qualification}
package=$(mktemp -d "${TMPDIR:-/tmp}/nano-kurtosis-XXXXXX")
cp "$root/tests/kurtosis/main.star" "$root/tests/kurtosis/kurtosis.yml" "$root/tests/kurtosis/checks.star" "$package/"
# Timestamped genesis must be generated immediately before deployment.
"$exporter" --export-kurtosis "$package/assets"
printf '{"image":"%s","witness_recovery":%s}\n' "$image" "${NANO_KURTOSIS_WITNESS_RECOVERY:-false}" > "$package/args.json"
printf 'Package retained for diagnosis: %s\n' "$package"
cd "$package"
"$kurtosis" run . --enclave "$enclave" --args-file "$package/args.json" --parallelism 2
