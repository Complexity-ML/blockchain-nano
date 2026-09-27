# Capella: local RANDAO and block proposal signing

2026-09-27, local macOS Release. Base `93db6b8`; hashes in `sources.json`.

## Implementation

`sign_randao` derives proposer identity and domain from the authenticated
current head and slot, checks the durable commitment and journal identity, and
serializes against journal retirement/signing. An epoch-only RANDAO signature
does not fabricate a block reservation.

`sign_proposal` fully constructs and executes the supplied body on the current
paired Beacon/EVM parent, including Capella withdrawals and consensus operations.
It resolves the signing key/domain from the resulting state and reserves the
actual Beacon header in the separate anti-slashing journal before signing.
Native-deposit profiles also require the body's deposit vote to be known.
Clock/token checks prevent stale work, and late provider results are withheld
without erasing the durable reservation. The same-slot journal refuses a
conflicting locally valid body after restart.

## Qualification

Four suites passed in 50.64 seconds. Across five baseline chains, 88 RANDAO
reveals and 88 locally signed blocks match the prior independent fixture
construction byte-for-byte and are then consumed by signed admission. This
covers three short chains (including post-migration continuation and skipped
fork slots), full-participation finality and half-participation non-finality.
The WAL fault scenario additionally repeats eight local proposals.

Each scenario tests an unassigned RANDAO identity without provider invocation.
At the first local proposal: stale token, invalid RANDAO body and cancellation
never reach the proposal provider; reopened journal accepts an identical block
with unchanged reservation, rejects changed graffiti, and a provider expiring
mid-signature leaves a reservation that still rejects the conflicting block.
Existing archive, reorg, withdrawal, crash, vote and attestation gates remain in
these runs.

Build `nano-capella-transition-tests`; CTest expression:
`^consensus_capella_(reactor_sync_failure|signed_transition|multiepoch_finality|insufficient_finality)$`.

## Limits

The caller still builds the body; automatic selection of transactions, votes,
proofs and exits is not wired. First-fork-block signing through a live mixed
Store is not covered: migration scenarios initialize a trusted Capella anchor,
then exercise local signing on its descendants. Native-deposit profile signing
is guarded in code but not independently qualified in this run. Scheduler,
network publication, owner integration and production key operations remain open.
No public activation or complete production claim.
