#!/bin/bash
set -eu
arch=$1
base=/build/coherent-efbeb9bc-$arch
parent=/build/export-contention-$arch-20261006
out=/build/incoming-alternation-$arch-20261006
mkdir "$out"
trap 'status=$?; echo "$status" > "$out/controller-exit.txt"' EXIT
sha256sum "$parent/libnano_core.a" > "$out/parent-before.sha256"
flags(){ sed -n 's/^CXX_\(DEFINES\|INCLUDES\|FLAGS\) = / /p' "$base/CMakeFiles/$1.dir/flags.make" | tr '\n' ' '; }
extra='-I/work/third_party/ckzg/blst/bindings -UNANO_TEST_SOURCE -DNANO_TEST_SOURCE=\"/work\"'
command="/usr/bin/clang++ -I/work/include $(flags nano_core) $extra -c /work/src/consensus_capella_reactor.cpp -o $out/consensus_capella_reactor.cpp.o"
printf '%s\n' "$command" > "$out/core-command.txt"
eval "$command" > "$out/core-compile.log" 2>&1
echo 0 > "$out/core-compile-exit.txt"
cp -p "$parent/libnano_core.a" "$out/libnano_core.a"
ar r "$out/libnano_core.a" "$out/consensus_capella_reactor.cpp.o"
ranlib "$out/libnano_core.a"
command="/usr/bin/clang++ -I/work/include $(flags nano-capella-transition-tests) $extra -c /work/tests/test_capella_transition.cpp -o $out/transition.o"
printf '%s\n' "$command" > "$out/test-command.txt"
eval "$command" > "$out/test-compile.log" 2>&1
echo 0 > "$out/test-compile-exit.txt"
for variant in baseline candidate; do
 command=$(cat "$base/CMakeFiles/nano-capella-transition-tests.dir/link.txt")
 command=${command//CMakeFiles\/nano-capella-transition-tests.dir\/tests\/test_capella_transition.cpp.o/$out\/transition.o}
 command=${command//-o nano-capella-transition-tests /-o $out\/$variant }
 if [ "$variant" = baseline ];then library=$parent/libnano_core.a;else library=$out/libnano_core.a;fi
 command=${command// libnano_core.a / $library }
 printf '%s\n' "$command" > "$out/$variant-link-command.txt"
 (cd "$base";eval "$command") > "$out/$variant-link.log" 2>&1
 echo 0 > "$out/$variant-link-exit.txt"
done
set +e
"$out/baseline" --incoming-alternation > "$out/baseline-run.log" 2>&1
status=$?
set -e
echo "$status" > "$out/baseline-run-exit.txt"
[ "$status" = 1 ]
grep -F 'Alternating valid blocks erased incoming target continuation' "$out/baseline-run.log"
for mode in incoming-alternation prepared-block wire; do
 set +e
 "$out/candidate" "--$mode" > "$out/candidate-$mode.log" 2>&1
 status=$?
 set -e
 echo "$status" > "$out/candidate-$mode-exit.txt"
 tail -5 "$out/candidate-$mode.log"
 [ "$status" = 0 ]
done
sha256sum -c "$out/parent-before.sha256" > "$out/parent-unchanged.log"
sha256sum /work/src/consensus_capella_reactor.cpp /work/include/nano/consensus_capella_reactor.hpp /work/tests/test_capella_transition.cpp > "$out/sources.sha256"
sha256sum "$out/libnano_core.a" "$out/baseline" "$out/candidate" > "$out/programs.sha256"
