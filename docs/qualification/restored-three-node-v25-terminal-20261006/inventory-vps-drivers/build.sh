#!/bin/bash
set -eu
base=/build/coherent-efbeb9bc-amd64
out=/build/v25-inventory-diagnostic-amd64-20261006
core=/build/network-status-coherent-amd64-20261006/libnano_core.a
mkdir -p "$out"
trap 'status=$?; echo "$status" > /drivers/proof/build-exit.txt' EXIT
expected=edf60e3e4983b193a98b087a348bedcd7fd2f4fd410e4bc576edb3b07237b656
actual=$(sha256sum "$core" | cut -d' ' -f1)
[ "$actual" = "$expected" ]
flags=$(sed -n 's/^CXX_\(DEFINES\|INCLUDES\|FLAGS\) = / /p' "$base/CMakeFiles/nano_core.dir/flags.make" | tr '\n' ' ')
command="/usr/bin/clang++ -I/work/include -I/work/third_party/ckzg/blst/bindings $flags -c /drivers/source/tests/capella/paused_capture.cpp -o $out/reader.o"
printf '%s\n' "$command" > /drivers/proof/compile-command.txt
eval "$command" > /drivers/proof/compile.log 2>&1
command=$(cat "$base/CMakeFiles/nano-capella-transition-tests.dir/link.txt")
command=${command//CMakeFiles\/nano-capella-transition-tests.dir\/tests\/test_capella_transition.cpp.o/$out\/reader.o}
command=${command//-o nano-capella-transition-tests /-o $out\/reader }
command=${command// libnano_core.a / $core }
printf '%s\n' "$command" > /drivers/proof/link-command.txt
(cd "$base";eval "$command") > /drivers/proof/link.log 2>&1
sha256sum "$out/reader" "$core" > /drivers/proof/programs.sha256
