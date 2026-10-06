#!/bin/bash
set -eu
arch=$1
base=/build/coherent-efbeb9bc-$arch
out=/build/parent-cache-$arch-20261006
trap 'status=$?; echo "$status" > "$out/regression-controller-exit.txt"' EXIT
flags=$(sed -n 's/^CXX_\(DEFINES\|INCLUDES\|FLAGS\) = / /p' "$base/CMakeFiles/nano-capella-transition-tests.dir/flags.make" | tr '\n' ' ')
extra='-I/work/include -I/work/third_party/ckzg/blst/bindings -UNANO_TEST_SOURCE -DNANO_TEST_SOURCE=\"/work\"'
command="/usr/bin/clang++ $extra $flags -c /work/tests/test_capella_transition.cpp -o $out/transition.o"
printf '%s\n' "$command" > "$out/test-command.txt"
eval "$command" > "$out/test-compile.log" 2>&1
command=$(cat "$base/CMakeFiles/nano-capella-transition-tests.dir/link.txt")
command=${command//CMakeFiles\/nano-capella-transition-tests.dir\/tests\/test_capella_transition.cpp.o/$out\/transition.o}
command=${command//-o nano-capella-transition-tests /-o $out\/candidate }
command=${command// libnano_core.a / $out\/libnano_core.a }
printf '%s\n' "$command" > "$out/test-link-command.txt"
(cd "$base";eval "$command") > "$out/test-link.log" 2>&1
for mode in incoming-alternation vote-target-alternation prepared-block wire; do
 set +e
 "$out/candidate" "--$mode" > "$out/candidate-$mode.log" 2>&1
 status=$?
 set -e
 echo "$status" > "$out/candidate-$mode-exit.txt"
 tail -4 "$out/candidate-$mode.log"
 [ "$status" = 0 ]
done
sha256sum "$out/libnano_core.a" "$out/candidate" > "$out/candidate-programs.sha256"
