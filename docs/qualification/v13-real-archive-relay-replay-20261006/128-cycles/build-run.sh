#!/bin/bash
set -eu
base=/build/coherent-efbeb9bc-arm64
core=/build/parent-cache-arm64-20261006/libnano_core.a
trap 'status=$?; echo "$status" > /output/controller-exit.txt' EXIT
flags=$(sed -n 's/^CXX_\(DEFINES\|INCLUDES\|FLAGS\) = / /p' "$base/CMakeFiles/nano_core.dir/flags.make" | tr '\n' ' ')
command="/usr/bin/clang++ -I/work/include -I/work/third_party/ckzg/blst/bindings $flags -I/work/network -DBOOST_ERROR_CODE_HEADER_ONLY -pthread -c /output/diagnose.cpp -o /output/diagnose.o"
printf '%s\n' "$command" > /output/compile-command.txt
eval "$command" > /output/compile.log 2>&1
echo 0 > /output/compile-exit.txt
command=$(cat "$base/CMakeFiles/nano-capella-transition-tests.dir/link.txt")
command=${command//CMakeFiles\/nano-capella-transition-tests.dir\/tests\/test_capella_transition.cpp.o/\/output\/diagnose.o}
command=${command//-o nano-capella-transition-tests /-o \/output\/diagnose }
command=${command// libnano_core.a / $core }
printf '%s\n' "$command" > /output/link-command.txt
(cd "$base";eval "$command") > /output/link.log 2>&1
echo 0 > /output/link-exit.txt
sha256sum /output/diagnose "$core" > /output/programs.sha256
/output/diagnose /output/request.json > /output/diagnostic.json 2> /output/error.log
echo 0 > /output/run-exit.txt
