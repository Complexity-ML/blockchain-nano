#!/bin/bash
set -eu
base=/build/finalized-retention-4734f285-amd64
out=/build/monotonic-clock-amd64-20261005
proof=$out/slashing-audit-control-20261005
mkdir "$proof"
trap 'helper_exit=$?; echo "$helper_exit" > "$proof/controller-exit.txt"' EXIT
[ ! -e "$out/nano-capella-slashing-audit" ]
sha256sum "$out/nano" "$out/libnano_core.a" "$out/nano-capella-transition-tests" "$out/network/nano-p2p" "$out/nano-capella-rotation-audit" "$out/nano-capella-slashing-fixture" > "$proof/runtime-before.sha256"
flags() { sed -n 's/^CXX_\(DEFINES\|INCLUDES\|FLAGS\) = / /p' "$base/CMakeFiles/nano_core.dir/flags.make" | tr '\n' ' '; }
eval "/usr/bin/clang++ $(flags) -c /work/tests/capella/slashing_audit.cpp -o $out/slashing-audit.cpp.o" > "$proof/compile.log" 2>&1
cd "$base"
command=$(cat CMakeFiles/nano.dir/link.txt)
command=${command//CMakeFiles\/nano.dir\/src\/main.cpp.o/$out\/slashing-audit.cpp.o}
command=${command//-o nano /-o $out\/nano-capella-slashing-audit }
command=${command// libnano_core.a / $out\/libnano_core.a }
eval "$command" > "$proof/link.log" 2>&1
sha256sum -c "$proof/runtime-before.sha256" > "$proof/runtime-unchanged.log"
sha256sum /work/tests/capella/slashing_audit.cpp "$out/nano-capella-slashing-audit" > "$proof/source-and-helper.sha256"
