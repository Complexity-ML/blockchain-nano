#!/bin/bash
set -eu
base=/build/coherent-efbeb9bc-arm64
trap 'echo $? > /probe/syntax-fixture-v5-exit.txt' EXIT
flags=$(sed -n 's/^CXX_\(DEFINES\|INCLUDES\|FLAGS\) = / /p' "$base/CMakeFiles/nano-capella-transition-tests.dir/flags.make" | tr '\n' ' ')
flags=${flags//\/work\//\/source\/}
cmd="/usr/bin/clang++ -I/source/include -I/source/network -I/source/third_party/ckzg/blst/bindings $flags -ferror-limit=0 -fsyntax-only /source/tests/test_capella_transition.cpp"
printf '%s\n' "$cmd" > /probe/syntax-fixture-v5-command.txt
eval "$cmd" > /probe/syntax-fixture-v5.log 2>&1
