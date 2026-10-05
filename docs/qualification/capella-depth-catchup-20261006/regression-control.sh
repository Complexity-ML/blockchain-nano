#!/bin/bash
set -eu
base=/build/coherent-efbeb9bc-arm64/network
for pair in 'head-catalog:test_capella_head_catalog_relay.cpp' 'catalog-relay:test_capella_catalog_relay.cpp' 'block-delivery:test_capella_block_delivery.cpp';do
 name=${pair%%:*};source=${pair#*:};target=nano-capella-$name-tests;out=/repro/fixed/$target
 flags=$(sed -n 's/^CXX_\(DEFINES\|INCLUDES\|FLAGS\) = / /p' "$base/CMakeFiles/$target.dir/flags.make" | tr '\n' ' ')
 eval "/usr/bin/clang++ $flags -I/repro/fixed -I/work/network -c /repro/fixed/$source -o $out.o" > "$out-compile.log" 2>&1
 command=$(cat "$base/CMakeFiles/$target.dir/link.txt")
 command=${command//CMakeFiles\/$target.dir\/$source.o/$out.o}
 command=${command//-o $target /-o $out }
 printf '%s\n' "$command" > "$out-link-command.txt"
 (cd "$base";eval "$command") > "$out-link.log" 2>&1
 "$out" > "$out-result.log" 2>&1
 cat "$out-result.log"
done
