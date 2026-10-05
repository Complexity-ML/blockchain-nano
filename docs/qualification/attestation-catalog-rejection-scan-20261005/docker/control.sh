#!/bin/bash
set -eu
base=$1
out=$2
mkdir "$out"
trap 'nano_scan_exit=$?; echo "$nano_scan_exit" > "$out/controller-exit.txt"' EXIT
sha256sum /work/network/capella_catalog_relay.hpp /work/network/test_capella_catalog_relay.cpp /work/network/main.cpp > "$out/sources.sha256"
for target in nano-capella-catalog-relay-tests nano-p2p; do
 flags=$(sed -n 's/^CXX_\(DEFINES\|INCLUDES\|FLAGS\) = / /p' "$base/CMakeFiles/$target.dir/flags.make" | tr '\n' ' ')
 source=/work/network/test_capella_catalog_relay.cpp
 if [ "$target" = nano-p2p ]; then source=/work/network/main.cpp; fi
 echo "compiling $target"
 eval "/usr/bin/clang++ $flags -c $source -o $out/$target.o" > "$out/$target-compile.log" 2>&1
 echo 0 > "$out/$target-compile-exit.txt"
 command=$(cat "$base/CMakeFiles/$target.dir/link.txt")
 object="CMakeFiles/$target.dir/$(basename "$source").o"
 command=${command//$object/$out\/$target.o}
 command=${command//-o $target /-o $out\/$target }
 printf '%s\n' "$command" > "$out/$target-link-command.txt"
 (cd "$base"; eval "$command") > "$out/$target-link.log" 2>&1
 echo 0 > "$out/$target-link-exit.txt"
 sha256sum "$out/$target" >> "$out/programs.sha256"
 if [ "$target" = nano-capella-catalog-relay-tests ]; then
  "$out/$target" > "$out/test.log" 2>&1
  echo 0 > "$out/test-exit.txt"
  cat "$out/test.log"
 fi
done
