#!/bin/bash
set -eu
sdk=/build/catalog-busy-integrated-arm64-20261006/network
out=/diag
flags=$(sed -n 's/^CXX_\(DEFINES\|INCLUDES\|FLAGS\) = / /p' "$sdk/CMakeFiles/nano-capella-dependency-preparation-tests.dir/flags.make" | tr '\n' ' ')
for kind in baseline candidate;do
 command="/usr/bin/clang++ -I/diag/$kind $flags -c /diag/trace.cpp -o /diag/$kind.o"
 printf '%s\n' "$command" > "/diag/$kind-compile-command.txt"
 eval "$command" > "/diag/$kind-compile.log" 2>&1
 link=$(cat "$sdk/CMakeFiles/nano-capella-dependency-preparation-tests.dir/link.txt")
 link=${link//CMakeFiles\/nano-capella-dependency-preparation-tests.dir\/test_capella_dependency_preparation.cpp.o/\/diag\/$kind.o}
 link=${link//-o nano-capella-dependency-preparation-tests /-o \/diag\/$kind-probe }
 printf '%s\n' "$link" > "/diag/$kind-link-command.txt"
 (cd "$sdk";eval "$link") > "/diag/$kind-link.log" 2>&1
 for mode in slow;do
  set +e
  "/diag/$kind-probe" "$mode" > "/diag/$kind-$mode.log" 2>&1
  code=$?
  set -e
  printf '%s\n' "$code" > "/diag/$kind-$mode-exit.txt"
  tail -2 "/diag/$kind-$mode.log"
 done
done
sha256sum /diag/baseline-probe /diag/candidate-probe > /diag/programs.sha256
