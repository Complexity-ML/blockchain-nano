# Capella: durable gossip votes and deferred admission

2026-09-27, local macOS Release. Base `628bae7`, changed sources identified in
`sources.json`.

## Implementation

The coordinator validates and durably records canonical signed gossip votes.
Current-slot votes are authenticated against the next-slot context but leave
current fork choice unchanged. A later tick applies them after their slot has
ended. Exact duplicates acknowledge existing delivery without rewriting the
checkpoint. Invalid signatures and cancellation publish nothing.

The canonical `nano-capella-durable-v1` local envelope binds Store and signed
votes under one digest/CAS token and atomic execution checkpoint. Empty archives
retain the previous Store format. Restart authenticates the outer digest before
parsing, validates each vote's SSZ/hash/BLS/context, rejects duplicate entries,
and requires canonical re-encoding. Archives are bounded by configured message
and envelope limits, and expire outside the current/previous epoch on tick.
Signed block archives continue using the immutable atomic storage records.

## Evidence

Four suites passed in 36.35 seconds. Eighteen immediately applicable votes
survive reopen across three branch/reorg scenarios. Twelve deferred votes
(eight/four at complete/half participation) persist across reopen, do not change
fork choice during their slot, and match pure next-slot processing after restart.
Their invalid-signature variants and cancellations are rejected; exact retries
preserve tokens. Advancing three epochs empties the signed vote archive.

A final full-participation run adds a valid restore control plus corrupted
signature and duplicate-entry checkpoints (their outer digests are recomputed,
so rejection tests inner validation). Passed in 14.22 seconds. The other baseline
suites were not repeated after this test-only addition and diagnostic wording.

Build `nano-capella-transition-tests`; run CTest expression
`^consensus_capella_(reactor_sync_failure|signed_transition|multiepoch_finality|insufficient_finality)$`.

## Limits

No network activation. This archives admitted gossip votes, not every embedded
block attestation, exit or slashing proof. Missing-parent votes are not queued.
Startup still needs an independently trusted token; there is no general rollback
recovery policy. WAL error injection in the baseline targets block publication,
not a separate vote-publication fault. Local signing, notifications, network
integration, mixed-fork migration and distributed qualification remain open.
