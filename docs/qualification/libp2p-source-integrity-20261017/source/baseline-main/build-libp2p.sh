#!/bin/sh
set -eu
source_only=${NANO_LIBP2P_SOURCE_ONLY-0}
case "$source_only" in
    0|1) ;;
    *) echo "NANO_LIBP2P_SOURCE_ONLY must be 0 or 1" >&2; exit 2 ;;
esac
root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
deps=${NANO_DEPS_DIR:-"$root/.deps"}
cmake_bin=${CMAKE_BIN:-cmake}
source_dir="$deps/cpp-libp2p"
commit=c03bf7c44c4fd30a390f3ef93bd1eab482e686a6
mkdir -p "$deps"
if [ ! -d "$source_dir/.git" ]; then
    git clone --depth 1 --branch v0.1.37 https://github.com/libp2p/cpp-libp2p.git "$source_dir"
fi
if [ "$(git -C "$source_dir" rev-parse HEAD)" != "$commit" ]; then
    echo "cpp-libp2p differs from dependencies.lock.json" >&2
    exit 1
fi
# Keep dependency caches local; never upload build artifacts to upstream.
export HUNTER_ROOT="$deps/hunter"
# Older bundled fmt uses consteval constructs rejected by AppleClang 21.
# Apply through compiler flags so Hunter re-extraction cannot discard the fix.
fmt_flags=""
if [ "$(uname -s)" = Darwin ]; then
    fmt_flags="-DFMT_CONSTEVAL="
fi
set --
if [ "$source_only" = 1 ]; then
    set -- -DHUNTER_USE_CACHE_SERVERS=NO -DHUNTER_KEEP_PACKAGE_SOURCES=ON \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
fi
"$cmake_bin" -S "$source_dir" -B "$deps/libp2p-build" \
    -DCMAKE_TOOLCHAIN_FILE="$root/cmake/libp2p-toolchain.cmake" \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$deps/install" \
    -DCMAKE_CXX_FLAGS="$fmt_flags" -DTESTING=OFF -DEXAMPLES=OFF -DCLANG_FORMAT=OFF -DHUNTER_RUN_UPLOAD=OFF "$@"
"$cmake_bin" --build "$deps/libp2p-build" --parallel "${JOBS:-4}"
"$cmake_bin" --install "$deps/libp2p-build"
