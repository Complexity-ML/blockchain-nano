#!/bin/bash
set -eu
arch=$1
base=/build/coherent-efbeb9bc-$arch
parent=/build/capacity-pressure-$arch-5902c983-20261006
out=/build/atomic-proposal-$arch-d71f574a-20261006
mkdir "$out"
trap 'status=$?; echo "$status" > "$out/controller-exit.txt"' EXIT
sha256sum "$parent/libnano_core.a" "$parent/fixed" > "$out/parent-before.sha256"
flags(){ sed -n 's/^CXX_\(DEFINES\|INCLUDES\|FLAGS\) = / /p' "$base/CMakeFiles/$1.dir/flags.make" | tr '\n' ' '; }
for unit in consensus_capella_signing consensus_capella_reactor consensus_capella_validator; do
 command="/usr/bin/clang++ $(flags nano_core) -I/work/third_party/ckzg/blst/bindings -c /work/src/$unit.cpp -o $out/$unit.cpp.o"
 printf '%s\n' "$command" > "$out/$unit-command.txt"
 eval "$command" > "$out/$unit-compile.log" 2>&1
 echo 0 > "$out/$unit-compile-exit.txt"
done
cp -p "$parent/libnano_core.a" "$out/libnano_core.a"
/usr/bin/ar r "$out/libnano_core.a" "$out/consensus_capella_signing.cpp.o" "$out/consensus_capella_reactor.cpp.o" "$out/consensus_capella_validator.cpp.o"
/usr/bin/ranlib "$out/libnano_core.a"
command="/usr/bin/clang++ $(flags nano-capella-transition-tests) -I/work/third_party/ckzg/blst/bindings -c /work/tests/test_capella_transition.cpp -o $out/transition.o"
printf '%s\n' "$command" > "$out/test-command.txt"
eval "$command" > "$out/test-compile.log" 2>&1
echo 0 > "$out/test-compile-exit.txt"
command=$(cat "$base/CMakeFiles/nano-capella-transition-tests.dir/link.txt")
command=${command//CMakeFiles\/nano-capella-transition-tests.dir\/tests\/test_capella_transition.cpp.o/$out\/transition.o}
command=${command//-o nano-capella-transition-tests /-o $out\/test }
command=${command// libnano_core.a / $out\/libnano_core.a }
printf '%s\n' "$command" > "$out/test-link-command.txt"
(cd "$base";eval "$command") > "$out/test-link.log" 2>&1
echo 0 > "$out/test-link-exit.txt"
for mode in proposal-gap atomic-proposal capacity-pressure scheduler scheduler-registration; do
 set +e
 "$out/test" "--$mode" > "$out/$mode.log" 2>&1
 status=$?
 set -e
 echo "$status" > "$out/$mode-exit.txt"
 cat "$out/$mode.log"
 if [ "$mode" = proposal-gap ];then [ "$status" = 1 ];grep -F 'two-step proposal lost admission capacity' "$out/$mode.log";else [ "$status" = 0 ];fi
done
sha256sum -c "$out/parent-before.sha256" > "$out/parent-unchanged.log"
sha256sum /work/src/consensus_capella_signing.cpp /work/src/consensus_capella_reactor.cpp /work/src/consensus_capella_validator.cpp /work/include/nano/consensus_capella_reactor.hpp /work/tests/test_capella_transition.cpp > "$out/sources.sha256"
sha256sum "$out/libnano_core.a" "$out/test" > "$out/programs.sha256"
