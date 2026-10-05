#!/bin/bash
set -eu
base=/build/coherent-efbeb9bc-arm64/network
out=/repro
flags=$(sed -n 's/^CXX_\(DEFINES\|INCLUDES\|FLAGS\) = / /p' "$base/CMakeFiles/nano-capella-head-catalog-tests.dir/flags.make" | tr '\n' ' ')
eval "/usr/bin/clang++ $flags -I/work/network -c $out/test-deep-head.cpp -o $out/test.o" > "$out/compile.log" 2>&1
command=$(cat "$base/CMakeFiles/nano-capella-head-catalog-tests.dir/link.txt")
command=${command//CMakeFiles\/nano-capella-head-catalog-tests.dir\/test_capella_head_catalog_relay.cpp.o/$out\/test.o}
command=${command//-o nano-capella-head-catalog-tests /-o $out\/test }
printf '%s\n' "$command" > "$out/link-command.txt"
(cd "$base";eval "$command") > "$out/link.log" 2>&1
set +e
"$out/test" > "$out/reproduction.log" 2>&1
status=$?
set -e
printf '%s\n' "$status" > "$out/reproduction-exit.txt"
cat "$out/reproduction.log"
[ "$status" = 1 ]
grep -F 'deep head blocks all historical catalog progress on a fresh peer' "$out/reproduction.log"
sha256sum /work/network/capella_head_catalog_relay.hpp /work/network/capella_block_delivery.hpp /work/network/capella_catalog_relay.hpp "$out/test-deep-head.cpp" "$out/test" > "$out/hashes.sha256"
