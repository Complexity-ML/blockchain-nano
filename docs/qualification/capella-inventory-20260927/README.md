# Capella checkpoint-fenced inventory — 2026-09-27

Base revision `f85daf2` plus accompanying changes.

Inventory walks ordered root maps with upper_bound cursors, under the reactor
mutex. It bounds both entries and serialized JSON bytes without copying all
payloads or building a full catalog. Continuations require an exact checkpoint;
stale checkpoints and unknown cursors are rejected. The trusted anchor is omitted.
Block entries are known candidates: export_message determines signed availability,
which may be absent for a trusted initial snapshot. Votes/slashings list retained
admitted messages. Root ordering is not parent-first synchronization ordering.

The enriched wire test passed in 5.28 seconds. Pages of at most two entries and
512 JSON bytes enumerate all 16 retained messages after two reorgs; each candidate
exports successfully and no duplicate occurs. Tiny byte budget, stale checkpoint,
continuation without checkpoint and unknown cursor are refused. Existing export,
admission, cancellation and persistence checks in this scenario also pass.

This is a local inventory primitive, not complete synchronization. Active clock
or state changes invalidate continuation: the future network sync layer needs a
bounded stable catalog/snapshot strategy to avoid repeated restarts under activity.
Parent dependency scheduling, RPC authentication/dispatch and libp2p remain open.
No network run, public activation, real funds or production-readiness claim.
