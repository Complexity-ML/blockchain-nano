#!/bin/bash
set -eu
base=/build/coherent-efbeb9bc-arm64/network
out=/build/export-resume-fixed-current-arm64-9ebc4cec-20261006
mkdir "$out"
trap 'status=$?; echo "$status" > "$out/controller-exit.txt"' EXIT
flags=$(sed -n 's/^CXX_\(DEFINES\|INCLUDES\|FLAGS\) = / /p' "$base/CMakeFiles/nano-capella-block-delivery-tests.dir/flags.make" | tr '\n' ' ')
for mode in fixed;do
 source=/diagnostic/baseline/test_capella_export_resume.cpp
 define=
 if [ "$mode" = fixed ];then source=/diagnostic/network/test_capella_export_resume.cpp;define=-DNANO_TEST_EXPORT_CACHE;fi
 command="/usr/bin/clang++ $flags $define -c $source -o $out/$mode.o"
 printf '%s\n' "$command" > "$out/$mode-compile-command.txt"
 eval "$command" > "$out/$mode-compile.log" 2>&1
 echo 0 > "$out/$mode-compile-exit.txt"
 command=$(cat "$base/CMakeFiles/nano-capella-block-delivery-tests.dir/link.txt")
 command=${command//CMakeFiles\/nano-capella-block-delivery-tests.dir\/test_capella_block_delivery.cpp.o/$out\/$mode.o}
 command=${command//-o nano-capella-block-delivery-tests /-o $out\/$mode }
 printf '%s\n' "$command" > "$out/$mode-link-command.txt"
 (cd "$base";eval "$command") > "$out/$mode-link.log" 2>&1
 echo 0 > "$out/$mode-link-exit.txt"
 set +e
 "$out/$mode" > "$out/$mode-run.log" 2>&1
 status=$?
 set -e
 echo "$status" > "$out/$mode-run-exit.txt"
 cat "$out/$mode-run.log"
 if [ "$mode" = baseline ];then [ "$status" = 1 ];else [ "$status" = 0 ];fi
done
sha256sum /diagnostic/network/*.hpp /diagnostic/network/main.cpp /diagnostic/network/test_capella_export_resume.cpp > "$out/sources.sha256"
echo 'Scoped before/after preparation regression completed; no image rebuild'
