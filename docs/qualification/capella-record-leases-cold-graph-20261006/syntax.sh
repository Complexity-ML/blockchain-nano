#!/bin/bash
set -eu
base=/build/coherent-efbeb9bc-arm64
trap 'echo $? > /probe/syntax-controller-exit.txt' EXIT
flags=$(sed -n 's/^CXX_\(DEFINES\|INCLUDES\|FLAGS\) = / /p' "$base/CMakeFiles/nano_core.dir/flags.make" | tr '\n' ' ')
flags=${flags//\/work\//\/source\/}
for path in /source/src/consensus_capella_*.cpp /source/src/capella_beacon_api.cpp;do
 name=$(basename "$path" .cpp)
 cmd="/usr/bin/clang++ -I/source/include -I/source/network -I/source/third_party/secp256k1/include -I/source/third_party/ckzg/blst/bindings $flags -fsyntax-only $path"
 printf '%s\n' "$cmd" > /probe/syntax-$name-command.txt
 eval "$cmd" > /probe/syntax-$name.log 2>&1
 echo "syntax $name"
done
