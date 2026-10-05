#!/bin/bash
set -eu
base=$1
parent=$2
out=$3
scope=$4
mkdir "$out"
trap 'status=$?; echo "$status" > "$out/controller-exit.txt"' EXIT
sha256sum "$parent/nano-p2p" > "$out/parent-before.sha256"
sha256sum /work/network/capella_head_catalog_relay.hpp /work/network/capella_block_delivery.hpp /work/network/capella_catalog_relay.hpp /work/network/test_capella_depth_catchup.cpp /work/network/main.cpp > "$out/sources.sha256"
for target in nano-capella-depth-catchup-tests nano-capella-head-catalog-tests nano-capella-catalog-relay-tests nano-capella-block-delivery-tests nano-p2p;do
 if [ "$scope" = arm64 ] && [ "$target" != nano-p2p ] && [ "$target" != nano-capella-depth-catchup-tests ];then continue;fi
 template=$target
 case "$target" in
 nano-capella-depth-catchup-tests) template=nano-capella-head-catalog-tests;source=test_capella_depth_catchup.cpp;object=test_capella_head_catalog_relay.cpp;;
 nano-capella-head-catalog-tests)source=test_capella_head_catalog_relay.cpp;object=$source;;
 nano-capella-catalog-relay-tests)source=test_capella_catalog_relay.cpp;object=$source;;
 nano-capella-block-delivery-tests)source=test_capella_block_delivery.cpp;object=$source;;
 nano-p2p)source=main.cpp;object=$source;;
 esac
 flags=$(sed -n 's/^CXX_\(DEFINES\|INCLUDES\|FLAGS\) = / /p' "$base/CMakeFiles/$template.dir/flags.make" | tr '\n' ' ')
 command="/usr/bin/clang++ $flags -I/work/network -c /work/network/$source -o $out/$target.o"
 printf '%s\n' "$command" > "$out/$target-compile-command.txt"
 eval "$command" > "$out/$target-compile.log" 2>&1
 echo 0 > "$out/$target-compile-exit.txt"
 command=$(cat "$base/CMakeFiles/$template.dir/link.txt")
 command=${command//CMakeFiles\/$template.dir\/$object.o/$out\/$target.o}
 command=${command//-o $template /-o $out\/$target }
 printf '%s\n' "$command" > "$out/$target-link-command.txt"
 (cd "$base";eval "$command") > "$out/$target-link.log" 2>&1
 echo 0 > "$out/$target-link-exit.txt"
 if [ "$target" = nano-p2p ];then "$out/$target" --help > "$out/$target-run.log" 2>&1
 else "$out/$target" > "$out/$target-run.log" 2>&1;cat "$out/$target-run.log";fi
 echo 0 > "$out/$target-run-exit.txt"
 sha256sum "$out/$target" >> "$out/programs.sha256"
done
sha256sum -c "$out/parent-before.sha256" > "$out/parent-unchanged.log"
echo 'scoped P2P candidate build completed without image rebuild'
