#!/bin/bash
set -eu
base=/build/coherent-efbeb9bc-arm64
trap 'echo $? > /probe/syntax-tools-controller-exit.txt' EXIT
flags=$(sed -n 's/^CXX_\(DEFINES\|INCLUDES\|FLAGS\) = / /p' "$base/CMakeFiles/nano_core.dir/flags.make" | tr '\n' ' ')
flags=${flags//\/work\//\/source\/}
for path in /source/tests/capella/*.cpp /source/tests/multihost/rotation_audit.cpp /source/tests/test_capella_audit_ancestry.cpp;do
 name=$(basename "$path" .cpp)
 cmd="/usr/bin/clang++ -I/source/include -I/source/network -I/source/third_party/ckzg/blst/bindings $flags -ferror-limit=0 -fsyntax-only $path"
 printf '%s\n' "$cmd" > /probe/syntax-tool-$name-command.txt
 eval "$cmd" > /probe/syntax-tool-$name.log 2>&1
 echo "syntax tool $name"
done
