# Capella inventory session quotas — 2026-09-27

Base revision `45c3ee1` plus accompanying changes.

CapellaInventorySessions retains immutable root catalogs behind random 256-bit
handles. An adapter-supplied authenticated principal owns each handle. Both handle
count and aggregate vector capacity bytes are bounded; fixed metadata overhead is
additionally bounded by handle count and a 256-byte principal length. Page reads
remain under the registry mutex so closing/expiry cannot release accounting while
a read still retains a snapshot. TTL uses steady time, is not refreshed by reads,
and expired entries are reclaimed lazily on operations. Backward clock readings
expire affected entries. Cancellation checks are forwarded to capture/page.

The wire test passed in 5.35 seconds. New assertions check owner reads, foreign
owner read/close denial, shared root-budget exhaustion, explicit close/reuse,
handle-count exhaustion, expiry and quota reclamation. Existing progression,
reorg, pagination, export and admission checks remain green.

This is a C++ registry for the private RPC adapter, not HTTP authentication itself.
The adapter must supply the principal from transport identity, enforce request
budgets/deadlines and configure quotas. Registry ownership/runtime configuration,
HTTP dispatch and distributed synchronization are still unwired. No funds or
public activation; no claim of production readiness.
