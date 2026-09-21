#!/bin/sh
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
deps=${NANO_DEPS_DIR:-"$root/.deps"}
cmake_bin=${CMAKE_BIN:-cmake}
"$cmake_bin" -S "$root/network" -B "$root/build-network" -DNANO_DEPS_DIR="$deps" -DCMAKE_BUILD_TYPE=Release
"$cmake_bin" --build "$root/build-network" --parallel "${JOBS:-4}"
