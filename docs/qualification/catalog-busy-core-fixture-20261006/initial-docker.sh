#!/bin/bash
set -eu
arch=$1
base=/build/coherent-efbeb9bc-$arch
core=/build/catalog-reservation-integrated-$arch-20261006
out=/work/$arch
mkdir -p "$out"
trap 'echo $? > "$out/controller-exit.txt"' EXIT
(cd /work/source;sha256sum -c /work/source.sha256) > "$out/source-before.log"
flags=$(awk '$1=="CXX_DEFINES" || $1=="CXX_INCLUDES" || $1=="CXX_FLAGS" {sub(/^[^=]*= /, ""); printf "%s ", $0}' "$base/CMakeFiles/nano-capella-transition-tests.dir/flags.make")
extra='-I/work/source/include -I/work/source/third_party/ckzg/blst/bindings -UNANO_TEST_SOURCE -DNANO_TEST_SOURCE=\"/work/source\"'
command="/usr/bin/clang++ $extra $flags -MMD -MF $out/candidate.d -c /work/source/tests/test_capella_transition.cpp -o $out/candidate.o"
printf '%s\n' "$command" > "$out/compile-command.txt"
eval "$command" > "$out/compile.log" 2>&1
command=$(cat "$base/CMakeFiles/nano-capella-transition-tests.dir/link.txt")
command=${command//CMakeFiles\/nano-capella-transition-tests.dir\/tests\/test_capella_transition.cpp.o/$out\/candidate.o}
command=${command//-o nano-capella-transition-tests /-o $out\/candidate }
command=${command// libnano_core.a / $core\/libnano_core.a }
printf '%s\n' "$command" > "$out/link-command.txt"
(cd "$base";eval "$command") > "$out/link.log" 2>&1
"$out/candidate" --wire > "$out/wire.log" 2>&1
echo 0 > "$out/wire-exit.txt"
sha256sum "$out/candidate" "$core/libnano_core.a" > "$out/programs.sha256"
(cd /work/source;sha256sum -c /work/source.sha256) > "$out/source-after.log"
tail -c 1800 "$out/wire.log"
