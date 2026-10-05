#!/bin/bash
set -eu
: "${NANO_AUDIT_BASE:?}" "${NANO_AUDIT_OUT:?}" "${NANO_AUDIT_P2P:?}"
base=$NANO_AUDIT_BASE
out=$NANO_AUDIT_OUT
proof=$out/rotation-auditor-control
mkdir "$proof"
trap 'nano_audit_exit=$?; echo "$nano_audit_exit" > "$proof/controller-exit.txt"' EXIT
sha256sum "$out/nano" "$out/libnano_core.a" "$out/nano-capella-transition-tests" > "$proof/runtime-before.sha256"
flags() { sed -n 's/^CXX_\(DEFINES\|INCLUDES\|FLAGS\) = / /p' "$base/CMakeFiles/nano_core.dir/flags.make" | tr '\n' ' '; }
eval "/usr/bin/clang++ $(flags) -c /work/tests/multihost/rotation_audit.cpp -o $out/rotation-audit.cpp.o" > "$proof/compile.log" 2>&1
cd "$base"
command=$(cat CMakeFiles/nano.dir/link.txt)
command=${command//CMakeFiles\/nano.dir\/src\/main.cpp.o/$out\/rotation-audit.cpp.o}
command=${command//-o nano /-o $out\/nano-capella-rotation-audit }
command=${command// libnano_core.a / $out\/libnano_core.a }
eval "$command" > "$proof/link.log" 2>&1
"$out/nano-capella-rotation-audit" --self-test > "$proof/self-test.log" 2>&1
sha256sum -c "$proof/runtime-before.sha256" > "$proof/runtime-unchanged.log"
sha256sum /work/tests/multihost/rotation_audit.cpp "$out/nano-capella-rotation-audit" "$NANO_AUDIT_P2P" > "$proof/source-and-programs.sha256"
mkdir -p "$out/network"
ln "$NANO_AUDIT_P2P" "$out/network/nano-p2p"
sha256sum "$out/network/nano-p2p" >> "$proof/source-and-programs.sha256"
