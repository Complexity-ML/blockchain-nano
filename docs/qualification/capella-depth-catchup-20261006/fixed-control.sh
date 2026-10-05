#!/bin/bash
set -eu
base=/build/coherent-efbeb9bc-arm64/network
out=/repro
flags=$(sed -n 's/^CXX_\(DEFINES\|INCLUDES\|FLAGS\) = / /p' "$base/CMakeFiles/nano-capella-head-catalog-tests.dir/flags.make" | tr '\n' ' ')
eval "/usr/bin/clang++ $flags -I/repro/fixed -I/work/network -c /repro/fixed/test-progressive.cpp -o /repro/fixed/test.o" > /repro/fixed/compile.log 2>&1
command=$(cat "$base/CMakeFiles/nano-capella-head-catalog-tests.dir/link.txt")
command=${command//CMakeFiles\/nano-capella-head-catalog-tests.dir\/test_capella_head_catalog_relay.cpp.o//repro\/fixed\/test.o}
command=${command//-o nano-capella-head-catalog-tests /-o \/repro\/fixed\/test }
printf '%s\n' "$command" > /repro/fixed/link-command.txt
(cd "$base";eval "$command") > /repro/fixed/link.log 2>&1
/repro/fixed/test > /repro/fixed/result.log 2>&1
cat /repro/fixed/result.log
