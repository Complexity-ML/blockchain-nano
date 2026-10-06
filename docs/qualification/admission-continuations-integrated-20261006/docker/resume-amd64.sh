#!/bin/bash
set -eu
out=/build/admission-continuations-amd64-20261006
trap 'status=$?; echo "$status" > /drivers/amd64-resume-controller-exit.txt' EXIT
ulimit -n > /drivers/amd64-resume-nofile.txt
for mode in automatic-recovery http; do
 set +e
 "$out/candidate" --$mode > "$out/test-$mode-resume.log" 2>&1
 status=$?
 set -e
 echo "$status" > "$out/test-$mode-resume-exit.txt"
 cat "$out/test-$mode-resume.log"
 [ "$status" -eq 0 ]
done
sha256sum "$out/candidate" "$out/libnano_core.a" > "$out/test-programs.sha256"
bash /drivers/sdk.sh amd64
