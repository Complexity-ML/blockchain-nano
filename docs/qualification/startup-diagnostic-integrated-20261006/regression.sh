#!/bin/bash
set -eu
arch=$1
base=/build/coherent-efbeb9bc-$arch
out=/build/startup-diagnostic-$arch-20261006
mkdir -p "$out"
core=/build/catalog-selection-$arch-20261006/libnano_core.a
trap 'status=$?; echo "$status" > "$out/regression-controller-exit.txt"' EXIT
flags=$(sed -n 's/^CXX_\(DEFINES\|INCLUDES\|FLAGS\) = / /p' "$base/CMakeFiles/nano-capella-transition-tests.dir/flags.make" | tr '\n' ' ')
extra='-I/work/include -I/work/third_party/ckzg/blst/bindings -UNANO_TEST_SOURCE -DNANO_TEST_SOURCE=\"/work\"'
command="/usr/bin/clang++ $extra $flags -c /work/tests/test_capella_transition.cpp -o $out/candidate.o"
printf '%s\n' "$command" > "$out/test-compile-command.txt"
eval "$command" > "$out/test-compile.log" 2>&1
command=$(cat "$base/CMakeFiles/nano-capella-transition-tests.dir/link.txt")
command=${command//CMakeFiles\/nano-capella-transition-tests.dir\/tests\/test_capella_transition.cpp.o/$out\/candidate.o}
command=${command//-o nano-capella-transition-tests /-o $out\/candidate }
command=${command// libnano_core.a / $core }
printf '%s\n' "$command" > "$out/test-link-command.txt"
(cd "$base";eval "$command") > "$out/test-link.log" 2>&1
for mode in wire;do
 "$out/candidate" --$mode > "$out/test-$mode.log" 2>&1
 echo 0 > "$out/test-$mode-exit.txt"
 tail -c 1100 "$out/test-$mode.log"
done
sha256sum "$out/candidate" "$core" > "$out/test-programs.sha256"
