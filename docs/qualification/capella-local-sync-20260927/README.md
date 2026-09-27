# Native Capella proposer sync aggregates — 2026-09-27

## Implementation

`CapellaDurableReactor::submit_sync_aggregate` accepts a complete SSZ sync
aggregate only for the current proposal slot and expected checkpoint. It checks
its exact bounded length before state work, authenticates the selected Capella
head and slot-advanced state, and validates the aggregate against the projected
Altair state (committee membership, fork/genesis domain, previous-slot block
root, signature and bit encoding). Signature validation is factored from reward
application, so admission does not accidentally reject a valid signature because
pre-block balances would overflow before withdrawals are processed. The official
runner now checks this distinction with saturated balances. The parent must precede the proposal slot.

One ephemeral aggregate is retained, bound to slot and selected parent root.
A byte-identical replay succeeds; another aggregate for that context replaces
it only if it has more participating positions. This bounds storage and prevents
a later empty aggregate from overwriting full participation. Cancellation before
publication leaves the previous selection intact. Durable state is unchanged.

Both direct local-body assembly and `propose_current` use the selected aggregate
only for its bound slot/head. Their existing complete block transition validates
it again before signing. Otherwise they retain the valid empty aggregate.
Restart drops this ephemeral cache; it does not discard validator signing
history. No new externally exposed RPC or P2P endpoint is added in this tranche.

## Qualification

The new local two-owner scenario supplies full BLS participation on even slots
and no aggregate on odd slots, asserting exact aggregate bytes in assembled
bodies. It builds signatures from the actual slot-advanced committee, including
repeated positions, and uses the previous-slot block root and appropriate fork
domain. `propose_current` signs the blocks; both reactors validate their state
roots and maintain finality/restart assertions from the common fixture.

Each submission cycle also rejects a corrupted signature, future slot and stale
checkpoint; accepts exact replay; refuses replacement by valid empty participation;
and checks cancellation and absence of durable writes. Odd slots must not reuse
an earlier cached aggregate. The official Altair sync-aggregate fixture runner
is executed alongside this integration test.

## Limits

Local complete-aggregate admission and assembly are implemented. Individual
message signing, collection, subnet aggregation, RPC/P2P transport and scheduler
wiring remain open. Overlapping partial aggregates are not merged. A competing
branch cache-invalidation scenario and restart between submission and proposal
still need dedicated evidence. This does not close the whole sync-participation
feature or production goal.

The first build caught a declaration inserted into the inventory class instead
of the reactor; it was moved before qualification. The error log is preserved.

## Result

An initial integration run passed in 35.78 s before the signature/reward split.
It is retained separately. Final qualification after that review change passed in 35.37 s combined:
34.91 s for the native loop (40 blocks, 20 full aggregates alternating with 20
empty aggregates, 272 attestation votes, finalized epoch 3, two owner reopens),
0.46 s for the official runner (16 valid, eight invalid fixtures, four signature
mutations, five context rejections and seven cancellation positions). The new
signature-only saturated-balance assertion also passed. Source/binary hashes
were unchanged afterward. Logs normalize trailing whitespace only.
