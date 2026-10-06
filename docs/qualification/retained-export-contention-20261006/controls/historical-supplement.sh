#!/bin/bash
set -eu
arch=$1
out=/build/export-contention-$arch-20261006
[ "$(cat "$out/controller-exit.txt")" = 1 ]
[ "$(cat "$out/candidate-beacon-reads-exit.txt")" = 0 ]
[ "$(cat "$out/candidate-wire-exit.txt")" = 0 ]
sha256sum "$out/candidate" "$out/libnano_core.a" > "$out/supplement-before.sha256"
set +e
"$out/candidate" --historical-export > "$out/candidate-historical-export-supplement.log" 2>&1
status=$?
set -e
echo "$status" > "$out/candidate-historical-export-supplement-exit.txt"
tail -5 "$out/candidate-historical-export-supplement.log"
sha256sum -c "$out/supplement-before.sha256" > "$out/supplement-programs-unchanged.log"
sha256sum -c "$out/parent-before.sha256" > "$out/parent-unchanged.log"
sha256sum /work/src/consensus_capella_reactor.cpp /work/include/nano/consensus_capella_reactor.hpp /work/tests/test_capella_transition.cpp > "$out/sources.sha256"
sha256sum "$out/libnano_core.a" "$out/baseline" "$out/candidate" > "$out/programs.sha256"
exit "$status"
