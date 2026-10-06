#!/bin/bash
set -eu
arch=$1
base=/build/coherent-efbeb9bc-$arch
out=/build/parent-cache-$arch-20261006
trap 'status=$?; echo "$status" > "$out/parent-regression-controller-exit.txt"' EXIT
flags=$(sed -n 's/^CXX_\(DEFINES\|INCLUDES\|FLAGS\) = / /p' "$base/CMakeFiles/nano-capella-transition-tests.dir/flags.make" | tr '\n' ' ')
extra='-I/work/third_party/ckzg/blst/bindings -UNANO_TEST_SOURCE -DNANO_TEST_SOURCE=\"/work\"'
for variant in baseline candidate; do
 headers=/work/include
 core=$out/libnano_core.a
 expected=0
 if [ "$variant" = baseline ];then headers=/baseline/include;core=/build/vote-target-cache-$arch-20261006/libnano_core.a;expected=1;fi
 command="/usr/bin/clang++ -I$headers $extra $flags -c /work/tests/test_capella_transition.cpp -o $out/parent-$variant.o"
 printf '%s\n' "$command" > "$out/parent-$variant-compile-command.txt"
 eval "$command" > "$out/parent-$variant-compile.log" 2>&1
 command=$(cat "$base/CMakeFiles/nano-capella-transition-tests.dir/link.txt")
 command=${command//CMakeFiles\/nano-capella-transition-tests.dir\/tests\/test_capella_transition.cpp.o/$out\/parent-$variant.o}
 command=${command//-o nano-capella-transition-tests /-o $out\/parent-$variant }
 command=${command// libnano_core.a / $core }
 printf '%s\n' "$command" > "$out/parent-$variant-link-command.txt"
 (cd "$base";eval "$command") > "$out/parent-$variant-link.log" 2>&1
 set +e
 "$out/parent-$variant" --incoming-parent-interference > "$out/parent-$variant-run.log" 2>&1
 status=$?
 set -e
 echo "$status" > "$out/parent-$variant-run-exit.txt"
 tail -4 "$out/parent-$variant-run.log"
 [ "$status" = "$expected" ]
 if [ "$variant" = baseline ];then grep -F 'Alternating parents erased incoming block continuation' "$out/parent-$variant-run.log";fi
done
sha256sum "$out/parent-baseline" "$out/parent-candidate" "$out/libnano_core.a" > "$out/parent-programs.sha256"
