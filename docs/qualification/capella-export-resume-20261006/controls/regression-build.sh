#!/bin/bash
set -eu
base=$1
out=$2
mkdir "$out"
trap 'status=$?; echo "$status" > "$out/controller-exit.txt"' EXIT
for target in nano-capella-block-delivery-tests nano-capella-head-catalog-tests nano-capella-attestation-delivery-tests nano-capella-catalog-relay-tests nano-capella-depth-catchup-tests;do
 case "$target" in
 nano-capella-block-delivery-tests)source=test_capella_block_delivery.cpp;;
 nano-capella-head-catalog-tests)source=test_capella_head_catalog_relay.cpp;;
 nano-capella-attestation-delivery-tests)source=test_capella_attestation_delivery.cpp;;
 nano-capella-catalog-relay-tests)source=test_capella_catalog_relay.cpp;;
 nano-capella-depth-catchup-tests)source=test_capella_depth_catchup.cpp;;
 esac
 template=$target
 original=$source
 if [ "$target" = nano-capella-depth-catchup-tests ];then template=nano-capella-head-catalog-tests;original=test_capella_head_catalog_relay.cpp;fi
 flags=$(sed -n 's/^CXX_\(DEFINES\|INCLUDES\|FLAGS\) = / /p' "$base/CMakeFiles/$template.dir/flags.make" | tr '\n' ' ')
 command="/usr/bin/clang++ $flags -c /diagnostic/network/$source -o $out/$target.o"
 printf '%s\n' "$command" > "$out/$target-compile-command.txt"
 eval "$command" > "$out/$target-compile.log" 2>&1
 echo 0 > "$out/$target-compile-exit.txt"
 command=$(cat "$base/CMakeFiles/$template.dir/link.txt")
 command=${command//CMakeFiles\/$template.dir\/$original.o/$out\/$target.o}
 command=${command//-o $template /-o $out\/$target }
 printf '%s\n' "$command" > "$out/$target-link-command.txt"
 (cd "$base";eval "$command") > "$out/$target-link.log" 2>&1
 echo 0 > "$out/$target-link-exit.txt"
 "$out/$target" > "$out/$target-run.log" 2>&1
 echo 0 > "$out/$target-run-exit.txt"
 cat "$out/$target-run.log"
done
sha256sum /diagnostic/network/capella_export_cache.hpp /diagnostic/network/capella_block_delivery.hpp /diagnostic/network/capella_head_catalog_relay.hpp /diagnostic/network/capella_attestation_delivery.hpp /diagnostic/network/main.cpp /diagnostic/network/test_capella_export_resume.cpp > "$out/sources.sha256"
echo 'Relevant ancestor/head/vote/catalog regressions passed; no network qualification'
