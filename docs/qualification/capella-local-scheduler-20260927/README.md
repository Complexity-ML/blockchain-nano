# Capella local clock and validator scheduler — 2026-09-27

Base revision `23926ae` plus accompanying sources.

## Implementation

`CapellaLocalClock` observes an independent local wall clock and latches backwards
or oversized forward steps and local errors. Cancellation does not poison it.
It coalesces consensus ticks within a slot/attestation-deadline phase while still
checking every wall-clock observation. Failure invalidates the execution owner.

`CapellaValidatorLoop` derives the proposer and committee from the authenticated
Store, proposes from the owner's mempool, then attests. Slot/epoch timing must
match the reactor profile. Transaction and vote assembly trials are configured.
Local journals prevent repeated duties, including after reopening; every result
re-enters ordinary signed block/vote admission. Proposal RANDAO, body construction
and header signing share the reactor lock and current commitment. Both classes
have stoppable/joined workers. This ports the existing Bellatrix scheduling model
without changing Bellatrix or exposing a new public activation path.

## New scenario

`consensus_capella_scheduled_finality`: one execution owner with 64 synthetic
local keys, controlled wall time, separate checkpoint witness. Forty transactions
are submitted through the owner mempool and the scheduler alone signs/adopts
40 blocks and 272 votes, finalizing epoch 3. Owner/coordinator/clock/scheduler and
64 signing journals are closed/reopened at slot 24; consensus resumes from the
trusted witness, journals from their independently retained exact commitments.

Repeated same-phase polls produce neither extra signatures nor checkpoint writes.
Wrong epoch timing is rejected. Cancellation preserves scheduler and clock health.
A real worker is started, observed polling, then stopped with completed duties and
no duplicate signatures. A backwards wall clock latches failure and closes owner
reads. The forty-slot run is driven by deterministic polls, not real-time sleeps.

## Limits

Single-process local scheduler qualification, not distributed consensus or P2P.
No Capella JSON startup/CLI, RPC worker, Beacon endpoint or key-loader integration
is claimed. Mixed-fork Store migration, exit/BLS-change queues, sync-contribution
production and native deposit qualification remain open. Clock forward-step
failure exists but is not independently exercised in this new scenario. Loss of
the witness's own WAL suffix and joint primary/witness rollback remain separate
recovery concerns. No public fork or real funds are activated.

## Results

Five suites passed in 56.28 seconds: signed transitions, owner reorg, owner sync
failure, witness recovery, and the new scheduled-finality scenario.
