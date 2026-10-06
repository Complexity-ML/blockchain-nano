#!/bin/bash
set -eu
arch=$1
base=/build/coherent-efbeb9bc-$arch
out=/build/provider-lifetime-$arch-20261006
trap 'echo $? > /probe/v2-controller-exit.txt' EXIT
flags=$(sed -n 's/^CXX_\(DEFINES\|INCLUDES\|FLAGS\) = / /p' "$base/CMakeFiles/nano_core.dir/flags.make" | tr '\n' ' ')
flags=${flags//\/work\//\/source\/}
cmd="/usr/bin/clang++ -I/source/include -I/source/network -I/source/third_party/ckzg/blst/bindings $flags -c /source/src/consensus_capella_checkpoint_provider.cpp -o $out/provider-v2.o"
printf '%s\n' "$cmd" > /probe/compile-v2-command.txt
eval "$cmd" > /probe/compile-v2.log 2>&1
cp "$out/libnano_core.a" "$out/libnano_core-v2.a"
cp "$out/provider-v2.o" "$out/consensus_capella_checkpoint_provider.cpp.o"
ar r "$out/libnano_core-v2.a" "$out/consensus_capella_checkpoint_provider.cpp.o"
ranlib "$out/libnano_core-v2.a"
cmd=$(cat /probe/link-command.txt)
cmd=${cmd//-o $out\/test /-o $out\/test-v2 }
cmd=${cmd// $out\/libnano_core.a / $out\/libnano_core-v2.a }
(cd "$base";eval "$cmd") > /probe/link-v2.log 2>&1
NANO_TEST_CHECKPOINT_PROVIDER=1 NANO_TEST_COMPACT_REFERENCE_RESTORE=1 NANO_TEST_COMPACT_REFERENCE_WRITER=1 NANO_TEST_FRAGMENT_CHECKPOINT=1 NANO_TEST_COLD_RECORD_CACHE=1 "$out/test-v2" --prepared-block > /probe/test-cold-graph-v2.log 2>&1
sha256sum "$out/test-v2" "$out/libnano_core-v2.a" > /probe/programs-v2.sha256
