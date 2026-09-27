# Capella ancestor-first delivery — 2026-09-27

Base revision `8ce7ba8` plus accompanying changes.

CapellaBlockDelivery walks the trusted local core archive from a requested block
to the authenticated anchor or a root acknowledged by the same peer/session.
It checks envelope identity, canonical SSZ header offsets, parent metadata against
the encoded parent and strictly decreasing slots. Root/body/signature validity is
provided by the local core archive verification and the receiving consensus
admission; this network helper is not a substitute for either.

Preparation bounds ancestor count and retained encoded-string capacity. The
transport must separately bound allocation before returning a parsed RPC response;
one response/decoded payload can coexist with the retained stack. Delivery starts
only after the dependency walk succeeds, oldest ancestor first. Positive ACKs alone
populate the bounded per-peer cache. Callers must serialize access to the cache
and discard it on peer/core session change. Eviction permits retransmission.
A lost reply/cancellation has unknown admission outcome and never caches a late
ACK. Previously acknowledged ancestors can remain admitted if a later block fails.

Thirteen scenarios passed in 0.31 s: normal and cached ancestry, duplicate replies,
missing parents, conflicting parent metadata, non-decreasing slots, malformed
SSZ offsets, foreign identity, count/byte budgets, rejected/lost ACKs, cancellation.
These use fake payloads/ACKs to qualify orchestration only.

The real two-reactor wire test passed in 5.73 s. It deliberately requests blocks
in reverse structural order through this helper, exports from the source archive
and awaits actual follower worker admission. Nine exports and nine block deliveries
are required, with no repeated acknowledged ancestor. After six votes and one
slashing, source/follower head, EVM state root, finalized checkpoint and retained
evidence agree. Existing structural root checks, two reorgs and durable reopen
checks remain in the surrounding scenario. Finalized epoch stays zero.

Commands:
```
cmake --build build-network --target nano-capella-block-delivery-tests -j4
ctest --test-dir build-network -R '^capella_block_delivery$' --output-on-failure
cmake --build build-release --target nano-capella-transition-tests -j4
ctest --test-dir build-release -R '^consensus_capella_wire_admission$' --output-on-failure
```

The core-export and peer-send callbacks are in-process in this qualification.
Daemon wiring, authenticated peer/session cache lifecycle, transport timeouts,
pruned-parent recovery and distributed advancing finality are still open.
No Capella network activation or real funds.
