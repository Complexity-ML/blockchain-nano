# Finality-gated Capella slashing archive retirement — 2026-09-27

## Change

On finalized-root advancement, publication examines the intersection of the two
indexed attestations in each stored proof. The proof remains if any implicated
validator is unknown or not marked slashed in the finalized state. It is removed
only when all implicated validators are finalized as slashed. The existing
atomic checkpoint path publishes archive and state together; the fork-choice
set of equivocating indices remains intact.

This bounds obsolete pending-proof retention without discarding evidence merely
because it was included on the current unfinalized head. Existing admission logic
also avoids re-adding evidence for an already finalized sanctioned validator.

## Qualification

A new native two-owner scenario submits a double-vote proof before the loop,
includes the slashing, and runs with automatic pruning over nine epochs. Every
produced block checks archive presence against the actual finalized validator
state on both owners. It requires observing both pre-finality retention and
post-finality retirement. Reversed-proof resubmission after retirement must not
change the checkpoint or grow the archive. Both databases are reopened afterward.

The test omits the sanctioned validator's attestations and leaves a slot empty
if the proposer-selection API refuses a selected proposer. It does not reroll
proposers. The existing post-pruning slashing test is rerun because publication
logic changed, checking evidence survives until a later inclusion is finalized.

## Remaining limits

The new test uses one offender. Multi-offender partial finalization, unknown
validator retention, competing branches/reorg of inclusion, and an injected
failure on the retirement write still need dedicated qualification. This is local
native consensus with synthetic keys, not a WAN or production-readiness result.
Global execution history and ancestor-pin disk growth remain separate open work.

Results: both tests passed in 111.00 s combined; the new retirement scenario took 69.18 s. It produced 72 blocks / 519 votes, finalized epoch 7, performed six automatic anchor moves and reopened both owners without the retired proof. No selected proposer was refused in this run, so the skip branch was not exercised. Source/binary hashes verified afterward; logs normalize trailing whitespace only.
