# Six real Capella nodes with an unresponsive peer — 2026-09-27

## Scenario and results

Command: `build-release/nano-capella-transition-tests --multipeer-cli
build-release/nano build-network/nano-p2p` (exit 0).

One native source node owns all 64 synthetic validator keys. Five independent
native follower nodes have separate databases/witnesses, no validator keys,
and bootstrap through the source's actual libp2p address. No shared follower
state or injected block data. This is six nodes, NOT six independent validators
or a distributed-stake finality qualification.

The first follower completes a catalog relay before its child `nano-p2p` is
paused with SIGSTOP. Its core remains running. Four other followers start, and
a 40-second observation window begins. All processes' cores must remain alive.
Each healthy follower must advance beyond the source's block at the window start
and match the source's canonical hash/stateRoot at its observed block number.
The source's real scheduler must finish attempts for block, attestation and
attester_slashing catalogs for every healthy peer. At least one attempt toward
the suspended peer must complete during the window.

Observed window: **40.097811167 s**; starting block **2**. Source completion
counts by peer: **[5, 98, 96, 96, 99]**, with the suspended peer first. All four
healthy followers progressed and all three kinds were scheduled for each.
After SIGCONT, all six nodes reached an identical current execution head hash
and stateRoot within the 60-second recovery bound. All exited cleanly.

Counts include successful and failed attempts; they are not counts of delivered
blocks or votes. The scheduler can time out/remove the unresponsive peer; this
test does not require keeping a stuck request alive for all 40 seconds. Progress
uses the complete production networking paths, so it is not attributed solely
to catalog delivery. No assertion claims universal scheduler fairness.

## Why there are two runs

The initial run passed catchup with counts [0,100,100,97,96]. It waited only for
peer discovery before SIGSTOP and thus did not prove an eligible relay toward
the paused peer had been exercised. The strengthened test waits for its first
completed catalog relay and requires a completion during the observation window.
Both exact logs and source patches are retained. Initial evidence is not silently
relabelled as the stronger qualification. No production logic was changed.

## Provenance and boundaries

`manifest.json` identifies final sources and binaries; their hashes were taken
during execution and rechecked afterward. `initial-manifest.json` identifies the
initial test source/binary, which differ from the final runner. Patches apply to
the recorded base with `git apply --unidiff-zero`. Gzip logs preserve exact bytes;
raw SHA-256 values and extracted results are in `summary.json`.

This narrows the previous registry-contention gap: the real peer scheduler
allowed four healthy competitors to advance with one unresponsive peer in this
bounded local run. It does not prove fairness for arbitrary peer counts, churn,
latency patterns, hostile identities, or bandwidth/load. SIGSTOP models a stalled
transport process, not asymmetric WAN packet loss. The registry's lack of mutex
fairness remains documented. Distributed finality, Docker/VPS behavior and broad
production readiness are unchanged by this test. No real funds or public network.
