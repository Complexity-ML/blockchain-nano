#!/bin/bash
set -eu
arch=$1
base=/build/coherent-efbeb9bc-$arch/network
parent=/build/head-fallback-p2p-$arch-20261006/nano-p2p
core=/build/parent-cache-$arch-20261006/nano
out=/build/history-first-p2p-$arch-20261006
mkdir "$out"
trap 'status=$?; echo "$status" > "$out/controller-exit.txt"' EXIT
sha256sum "$parent" "$core" > "$out/parents-before.sha256"
flags=$(sed -n 's/^CXX_\(DEFINES\|INCLUDES\|FLAGS\) = / /p' "$base/CMakeFiles/nano-p2p.dir/flags.make" | tr '\n' ' ')
command="/usr/bin/clang++ -I/output/network -I/work/third_party -I/work/include $flags -c /output/network/main.cpp -o $out/main.o"
printf '%s\n' "$command" > "$out/compile-command.txt"
eval "$command" > "$out/compile.log" 2>&1
echo 0 > "$out/compile-exit.txt"
command=$(cat "$base/CMakeFiles/nano-p2p.dir/link.txt")
command=${command//CMakeFiles\/nano-p2p.dir\/main.cpp.o/$out\/main.o}
command=${command//-o nano-p2p /-o $out\/nano-p2p }
printf '%s\n' "$command" > "$out/link-command.txt"
(cd "$base";eval "$command") > "$out/link.log" 2>&1
echo 0 > "$out/link-exit.txt"
set +e
"$out/nano-p2p" > "$out/usage.log" 2>&1
status=$?
set -e
echo "$status" > "$out/usage-exit.txt"
[ "$status" = 1 ]
grep -F 'Usage: nano-p2p' "$out/usage.log"
sha256sum -c "$out/parents-before.sha256" > "$out/parents-unchanged.log"
sha256sum "$out/nano-p2p" > "$out/program.sha256"
sha256sum /output/network/altair_core_dispatch.hpp /output/network/capella_block_delivery.hpp /output/network/capella_attestation_delivery.hpp /output/network/capella_head_catalog_relay.hpp /output/network/capella_relay_schedule.hpp /output/network/main.cpp > "$out/sources.sha256"
echo 'P2P candidate linked; expected missing-arguments exit 1; no live network qualification'
