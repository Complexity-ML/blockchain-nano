# FAILED: distributed Capella finality — 2026-09-27

Base revision `cd04918` plus the accompanying test. This is a production blocker,
not a successful qualification. The test deliberately remains failing until the
implementation meets its original criterion; it is an explicit long-running CLI
scenario, not silently skipped or converted to an expected failure.

Command:
```
build-release/nano-capella-transition-tests --distributed-cli build-release/nano build-network/nano-p2p
```

Two real supervisors/libp2p daemons run on loopback with disjoint groups of 32
synthetic validator keys out of 64. Neither process receives the other group's
secret environment variables. No artificial partition, load flood or public
network is involved. JSON Beacon API observations must reach a common finalized
checkpoint at epoch >=2, with corresponding finalized headers matching on both
nodes for three observations. The 210-second bound is preserved.

Observed result: exit 1 after the bound. Both workers remain healthy and produce
18/20 proposals and 125/122 local attestations respectively. Finality remains at
epoch 0; stopping receipts report justified epochs 1 and 2. Attestation traces
agree on head through slot 18 and diverge at slot 19. The last trace contains
64 latest-message records on each node; all 64 differ between nodes, with stale
remote validator epochs visible. Hundreds of catalog attempts report success,
but their delivered count includes cached entries and is not a count of newly
applied fork-choice votes. It cannot prove consensus progress.

`failed-run.log` preserves full process output and duty traces. `analysis.json`
extracts per-slot heads/sources/targets and differing latest messages. The fixture
cleans temporary stores on exit, so this run does not retain a complete SSZ archive
for exact offline replay. Do not infer a specific root cause from the traces alone.

Next investigation: trace fresh remote attestations through export, acknowledgement,
durable/deferred storage, next-slot application and latest-message updates; relate
the first divergence to block/justification transitions. Add a deterministic
regression when the failing transition is isolated, then rerun this unchanged
finality criterion. Increasing the timeout or weakening the criterion is not a fix.

No production readiness, distributed finality, partition recovery or WAN safety
is established by the earlier two-peer catch-up success. No live deployment changed.
