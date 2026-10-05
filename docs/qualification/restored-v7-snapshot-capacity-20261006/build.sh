#!/bin/bash
set -eu
parent=/build/deposit-deadline-amd64-b5ee00ac-20261005
out=/build/v7-snapshot-counts-amd64-f7785959-20261006
mkdir "$out"
trap 'status=$?; echo "$status" > "$out/controller-exit.txt"' EXIT
sha256sum "$parent/nano" "$parent/libnano_core.a" "$parent/nano-capella-paused-capture" > "$out/parent-before.sha256"
sha256sum /diagnostic/snapshot_counts.cpp > "$out/source.sha256"
command=$(cat "$parent/capture-compile-command.txt")
command=${command//\/work\/tests\/capella\/paused_capture.cpp/\/diagnostic\/snapshot_counts.cpp}
command=${command//-o $parent\/capture.o/-o $out\/counts.o}
printf '%s\n' "$command" > "$out/compile-command.txt"
eval "$command" > "$out/compile.log" 2>&1
echo 0 > "$out/compile-exit.txt"
command=$(cat "$parent/capture-link-command.txt")
command=${command//$parent\/capture.o/$out\/counts.o}
command=${command//$parent\/nano-capella-paused-capture/$out\/snapshot-counts}
printf '%s\n' "$command" > "$out/link-command.txt"
(cd /build/coherent-efbeb9bc-amd64; eval "$command") > "$out/link.log" 2>&1
echo 0 > "$out/link-exit.txt"
sha256sum -c "$out/parent-before.sha256" > "$out/parent-unchanged.log"
sha256sum "$out/snapshot-counts" > "$out/program.sha256"
echo 'Diagnostic-only tool built; runtime executables unchanged'
