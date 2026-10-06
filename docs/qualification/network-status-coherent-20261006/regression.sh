#!/bin/bash
set -eu
arch=$1
base=/build/coherent-efbeb9bc-$arch
out=/build/network-status-coherent-$arch-20261006
trap 'status=$?; echo "$status" > "$out/regression-controller-exit.txt"' EXIT
flags=$(sed -n 's/^CXX_\(DEFINES\|INCLUDES\|FLAGS\) = / /p' "$base/CMakeFiles/nano-capella-transition-tests.dir/flags.make" | tr '\n' ' ')
extra='-I/work/include -I/work/third_party/ckzg/blst/bindings -UNANO_TEST_SOURCE -DNANO_TEST_SOURCE=\"/work\"'
command="/usr/bin/clang++ $extra $flags -c /work/tests/test_capella_transition.cpp -o $out/candidate.o"
printf '%s\n' "$command" > "$out/test-compile-command.txt"
eval "$command" > "$out/test-compile.log" 2>&1
command=$(cat "$base/CMakeFiles/nano-capella-transition-tests.dir/link.txt")
command=${command//CMakeFiles\/nano-capella-transition-tests.dir\/tests\/test_capella_transition.cpp.o/$out\/candidate.o}
command=${command//-o nano-capella-transition-tests /-o $out\/candidate }
command=${command// libnano_core.a / $out\/libnano_core.a }
printf '%s\n' "$command" > "$out/test-link-command.txt"
(cd "$base";eval "$command") > "$out/test-link.log" 2>&1
for mode in beacon-reads configured-runtime automatic-recovery http wire;do
 "$out/candidate" --$mode > "$out/test-$mode.log" 2>&1
 echo 0 > "$out/test-$mode-exit.txt"
 tail -c 1100 "$out/test-$mode.log"
done
sha256sum "$out/candidate" "$out/libnano_core.a" > "$out/test-programs.sha256"

for field in identity sync;do
 NANO_TEST_NETWORK_STATUS_FIELD="$field" "$out/candidate" --beacon-reads > "$out/test-network-status-$field.log" 2>&1
 echo 0 > "$out/test-network-status-$field-exit.txt"
 tail -c 1000 "$out/test-network-status-$field.log"
done
