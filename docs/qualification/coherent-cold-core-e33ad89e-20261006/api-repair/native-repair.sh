#!/bin/bash
set -eu
root=/private/tmp/nano-coherent-e33ad89e-20261006
out=$root/api-repair/native
base=$root/native-build
mkdir "$out"
trap 'echo $? > "$out/controller-exit.txt"' EXIT
shasum -a 256 "$base/libnano_core.a" "$base/nano" > "$out/core-before.sha256"
/opt/homebrew/bin/cmake --build "$base" --target nano-capella-paused-capture nano-capella-deposit-audit --parallel 2 > "$out/remaining-tools.log" 2>&1
flags=$(sed -n 's/^CXX_\(DEFINES\|INCLUDES\|FLAGS\) = / /p' "$root/api-repair/native-flags.make" | tr '\n' ' ')
cmd="/usr/bin/clang++ $flags -c $root/api-repair/source/tests/test_capella_transition.cpp -o $out/test.o"
printf '%s\n' "$cmd" > "$out/compile-command.txt"
set +e; eval "$cmd" > "$out/compile.log" 2>&1; rc=$?; echo "$rc" > "$out/compile-exit.txt"; set -e; test "$rc" -eq 0
cmd=$(cat "$root/api-repair/native-link.txt")
cmd=${cmd//\"CMakeFiles\/nano-capella-transition-tests.dir\/tests\/test_capella_transition.cpp.o\"/$out\/test.o}
cmd=${cmd//-o nano-capella-transition-tests /-o $out\/test }
printf '%s\n' "$cmd" > "$out/link-command.txt"
(cd "$base"; eval "$cmd") > "$out/link.log" 2>&1
shasum -a 256 "$out/test" > "$out/program-before.sha256"
set +e
env NANO_TEST_REFERENCE_PREFLIGHT=1 NANO_TEST_REFERENCE_VALIDATION_MEMO=1 NANO_TEST_COMPACT_REFERENCE_RESTORE=1 NANO_TEST_COMPACT_REFERENCE_WRITER=1 NANO_TEST_COLD_RECORD_CACHE=1 NANO_TEST_FRAGMENT_CHECKPOINT=1 "$out/test" --prepared-block > "$out/writer-portability.log" 2>&1
rc=$?; echo "$rc" > "$out/writer-portability-exit.txt"; set -e; test "$rc" -eq 0
shasum -a 256 -c "$out/core-before.sha256" > "$out/core-unchanged.log"
shasum -a 256 -c "$out/program-before.sha256" > "$out/program-unchanged.log"
