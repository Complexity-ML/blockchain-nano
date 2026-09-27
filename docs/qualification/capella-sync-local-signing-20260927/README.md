# Local sync signing and scheduler — 2026-09-27

## Implemented

`sync_current` authenticates the next proposal slot's committee from the selected
Capella state advanced by one slot, uses the signed slot's fork version and sync
BLS domain, verifies the journal's chain/genesis/public key and serialization
against retirement/import, checks provider output, then invokes normal message
admission. It does not invent proposal or attestation slashing reservations for
sync signatures. A message already held for this member/slot/head is not signed
again. Key material remains in the existing provider.

Signing requires a current-slot head or the one-third-slot threshold for a skipped
slot. The local clock must remain monotonic and in the signed slot through provider
execution and admission checkpoints. Invalid/expired output is never exported.

The scheduler performs sync duties after proposals and attestations, preserving
attestation priority. It reports `syncMessagesAdmitted`. A compact committee/key
context is cached by head and signed slot; the authenticated registry is scanned
once per new context rather than per signing key. Incoming-message validation
still repeats bounded state work per member and needs performance qualification.

Reference: [Altair honest validator, consensus-specs v1.5.0](https://github.com/ethereum/consensus-specs/blob/v1.5.0/specs/altair/validator.md#prepare-sync-committee-message).

## Evidence

Release target `nano-capella-transition-tests` built. Command:
`ctest --test-dir build-release -R '^consensus_capella_(local_sync|scheduled_finality)$' -j 2 --output-on-failure`.

- Local sync: 48.46 s, 40 blocks, 272 votes, finality epoch 3, two RocksDB owners,
  two database reopens and 64 journals reopened. Production signing output matches
  independently assembled BLS signatures and aggregates. New negatives cover wrong
  key journal, cancellation, invalid provider output and provider crossing the slot
  deadline. Failed signing leaves no export. Successful sync signing leaves the
  journal commitment unchanged, duplicate duties never invoke the provider, and
  skipped-head signing waits until one third of the slot. Collection/export and
  exact next-block inclusion remain exercised across two reactor instances.
- Scheduled finality: 38.21 s, 40 blocks, 272 attestations, **1280 sync messages**,
  finality epoch 3, witness-backed reactor restart and 64 reopened signing journals.
  Every poll's produced block is followed by an exportable sync catalog; repeated
  polls do not sign or mutate durable state. Real scheduler thread start/stop and
  backward clock fail-closed behavior remain tested.
- 2/2 tests passed, elapsed 48.46 s in parallel. No unchanged broad suite rerun.

## Remaining gates

The clocks in these functional scenarios are synthetic. This does not establish
real-time deadline performance, actual libp2p/WAN message delivery, distributed
committee coverage, large registries, committee-period turnover or recovery during
an in-flight sync duty. No sync subnet aggregator/selection protocol added. No
production declaration, public launch, real funds or VPS deployment. Retired journal
rejection is implemented through the existing guard but not a new dedicated case
in this tranche. No new Engine API or fork activation claim.

`provenance.json` records source/binary hashes before testing and confirms them
afterward. `source.patch` includes the new signing source. Logs are normalized only
for trailing whitespace.
