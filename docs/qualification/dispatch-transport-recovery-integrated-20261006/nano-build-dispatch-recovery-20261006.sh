#!/bin/bash
set -eu
arch=$1
base=/build/coherent-efbeb9bc-$arch
out=/build/dispatch-recovery-$arch-1f521b7b-20261006-r2
mkdir "$out"
trap 'status=$?; echo "$status" > "$out/controller-exit.txt"' EXIT
core=/build/atomic-proposal-$arch-d71f574a-20261006/libnano_core.a
sha256sum "$core" > "$out/core-before.sha256"
for case in transport receipt http legacy;do
 template=$base/CMakeFiles/nano-altair-receipts-tests.dir
 original=CMakeFiles/nano-altair-receipts-tests.dir/tests/test_altair_receipts.cpp.o
 source=/diagnostic/tests/test_dispatch_${case}_recovery.cpp
 if [ "$case" = legacy ];then
 template=$base/network/CMakeFiles/nano-altair-core-dispatch-tests.dir
 original=CMakeFiles/nano-altair-core-dispatch-tests.dir/test_altair_core_dispatch.cpp.o
 source=/diagnostic/network/test_altair_core_dispatch.cpp
 fi
 flags=$(sed -n 's/^CXX_\(DEFINES\|INCLUDES\|FLAGS\) = / /p' "$template/flags.make" | tr '\n' ' ')
 command="/usr/bin/clang++ -I/diagnostic -I/diagnostic/network -I/diagnostic/third_party -I/diagnostic/include $flags -c $source -o $out/$case.o"
 printf '%s\n' "$command" > "$out/$case-compile-command.txt"
 eval "$command" > "$out/$case-compile.log" 2>&1
 command=$(cat "$template/link.txt")
 command=${command//$original/$out\/$case.o}
 if [ "$case" = legacy ];then
 command=${command//-o nano-altair-core-dispatch-tests /-o $out\/$case }
 linkdir=$base/network
 else
 command=${command//-o nano-altair-receipts-tests /-o $out\/$case }
 command=${command//libnano_core.a/$core}
 linkdir=$base
 fi
 printf '%s\n' "$command" > "$out/$case-link-command.txt"
 (cd "$linkdir";eval "$command") > "$out/$case-link.log" 2>&1
 "$out/$case" > "$out/$case-run.log" 2>&1
 echo 0 > "$out/$case-run-exit.txt"
 cat "$out/$case-run.log"
done
sha256sum -c "$out/core-before.sha256" > "$out/core-unchanged.log"
sha256sum /diagnostic/network/altair_core_dispatch.hpp /diagnostic/network/main.cpp /diagnostic/tests/test_dispatch* /diagnostic/network/test_altair_core_dispatch.cpp > "$out/sources.sha256"
sha256sum "$out/transport" "$out/receipt" "$out/http" "$out/legacy" > "$out/programs.sha256"
