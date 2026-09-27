# Capella: durable attester-slashing evidence

2026-09-27, local macOS Release; base `add0587`, source hashes in `sources.json`.

## Implementation

The coordinator persists accepted signed attester-slashing evidence atomically
with the fork-choice Store and its canonical execution suffix. Local envelope
`nano-capella-durable-v2` adds `signedSlashings`; the previous vote-only v1 and
plain Store formats remain readable and retain their byte encoding when used.

On restore, bounded SSZ decoding, content hash, signatures and slashing rules
are rechecked. Applying each archived proof must leave the snapshot's existing
equivocating-index set unchanged: a checkpoint cannot retain a valid proof while
silently removing the corresponding fork-choice exclusion.

New evidence is retained only when its signer intersection covers an offender
not already covered by retained proofs and not already slashed in finalized
state. Repeated, reversed or alternate signed pairs for the same covered offender
do not grow the archive or rewrite its checkpoint. Limits use the configured
equivocation/envelope budgets. Cancellation publishes no candidate.

## Qualification

Four suites passed in 38.82 seconds:

- signed transition / branch reorg / restart scenarios;
- complete-participation multi-epoch finality;
- insufficient-participation non-finality;
- uncertain WAL block-publication recovery.

Three proof archives survive reopen. Across those three scenarios, eighteen
repeated/reversed/alternate submissions preserve one proof and its token.
Three invalid signatures and three cancellations publish nothing.
Nine corrupted restart cases are rejected (bad proof signature with recomputed
hash, duplicate archive entry, missing equivocation exclusion); three unchanged
restart controls succeed. Vote-only and plain checkpoints remain exercised by
the other suites.

Build `nano-capella-transition-tests`, then run CTest expression
`^consensus_capella_(reactor_sync_failure|signed_transition|multiepoch_finality|insufficient_finality)$`.

## Limits and next integration

This persists gossip evidence and fork-choice exclusion. It does not itself
apply a financial penalty: on-chain slashing requires inclusion in a valid block
and the existing consensus state transition. Automatic proposer inclusion,
retirement of finalized consumed proofs, exit archives, local signing and
network integration remain open. The fault test still targets block publication,
not a separate slashing commit. No live network activation or production claim.
