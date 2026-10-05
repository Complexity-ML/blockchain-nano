#!/bin/bash
set -eu
base=/build/coherent-efbeb9bc-amd64
parent=/build/deposit-deadline-amd64-b5ee00ac-20261005
out=/build/capacity-pressure-amd64-5902c983-20261006
mkdir "$out"
trap 'status=$?; echo "$status" > "$out/controller-exit.txt"' EXIT
sha256sum "$parent/libnano_core.a" "$parent/nano" > "$out/parent-before.sha256"
flags(){ sed -n 's/^CXX_\(DEFINES\|INCLUDES\|FLAGS\) = / /p' "$base/CMakeFiles/$1.dir/flags.make" | tr '\n' ' '; }
for unit in consensus_capella_signing consensus_capella_reactor; do
 command="/usr/bin/clang++ $(flags nano_core) -I/work/third_party/ckzg/blst/bindings -c /work/src/$unit.cpp -o $out/$unit.cpp.o"
 printf '%s\n' "$command" > "$out/$unit-command.txt"
 eval "$command" > "$out/$unit-compile.log" 2>&1
 echo 0 > "$out/$unit-compile-exit.txt"
done
cp -p "$parent/libnano_core.a" "$out/libnano_core.a"
/usr/bin/ar r "$out/libnano_core.a" "$out/consensus_capella_signing.cpp.o" "$out/consensus_capella_reactor.cpp.o"
/usr/bin/ranlib "$out/libnano_core.a"
command="/usr/bin/clang++ $(flags nano-capella-transition-tests) -I/work/third_party/ckzg/blst/bindings -c /work/tests/test_capella_transition.cpp -o $out/transition.o"
printf '%s\n' "$command" > "$out/test-command.txt"
eval "$command" > "$out/test-compile.log" 2>&1
echo 0 > "$out/test-compile-exit.txt"
for mode in baseline fixed; do
 library="$parent/libnano_core.a";if [ "$mode" = fixed ]; then library="$out/libnano_core.a";fi
 command=$(cat "$base/CMakeFiles/nano-capella-transition-tests.dir/link.txt")
 command=${command//CMakeFiles\/nano-capella-transition-tests.dir\/tests\/test_capella_transition.cpp.o/$out\/transition.o}
 command=${command//-o nano-capella-transition-tests /-o $out\/$mode }
 command=${command// libnano_core.a / $library }
 printf '%s\n' "$command" > "$out/$mode-link-command.txt"
 (cd "$base";eval "$command") > "$out/$mode-link.log" 2>&1
 echo 0 > "$out/$mode-link-exit.txt"
 set +e
 "$out/$mode" --capacity-pressure > "$out/$mode-run.log" 2>&1
 status=$?
 set -e
 echo "$status" > "$out/$mode-run-exit.txt"
 cat "$out/$mode-run.log"
 if [ "$mode" = baseline ];then [ "$status" = 1 ];grep -F 'full graph invoked automatic proposal signer' "$out/$mode-run.log";else [ "$status" = 0 ];fi
done
for regression in configured-runtime scheduler; do
 "$out/fixed" "--$regression" > "$out/$regression.log" 2>&1
 echo 0 > "$out/$regression-exit.txt"
 cat "$out/$regression.log"
done
sha256sum -c "$out/parent-before.sha256" > "$out/parent-unchanged.log"
sha256sum /work/src/consensus_capella_signing.cpp /work/src/consensus_capella_reactor.cpp /work/tests/test_capella_transition.cpp > "$out/sources.sha256"
