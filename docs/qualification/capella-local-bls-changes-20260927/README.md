# Capella local selection of BLS withdrawal changes — 2026-09-27

## Implementation

`CapellaDurableReactor::prepare_local_body_with_bls_changes` accepts untrusted
signed candidates and an explicit positive trial budget. Candidate selection
reserves SSZ space before transaction selection, respects the profile's maximum
BLS-change list length, and appends changes after the execution payload. Each
addition must pass the existing complete unsigned Capella block transition.
Invalid signatures, duplicate credential changes and other invalid additions are
excluded. The candidate is not published; normal signing and block admission
remain required. Cancellation and stale-checkpoint checks still apply.

Reference: [Ethereum consensus-specs v1.6.0, Capella](https://github.com/ethereum/consensus-specs/blob/v1.6.0/specs/capella/beacon-chain.md),
`BeaconBlockBody`, `process_block`, `process_operations` and
`process_bls_to_execution_change`. Withdrawal processing precedes credential
changes; the genesis fork domain and withdrawal-key authorization are checked by
the existing full transition. No reinterpretation of validator signing keys.

## Tests and evidence

- `tests.log`: baseline signed-transition suite passed (15.10 s), including
  existing migration/skipped-slot scenarios. Initial candidate-selection scenario
  passed (4.98 s). This log predates the final test-fixture enhancement described
  below; production selection code was already the same.
- `withdrawal.log`: final `consensus_capella_local_bls_changes` passed (5.47 s).
  A second synthetic withdrawal key and validator receive an excess balance.
  The builder sees invalid-signature, valid, then duplicate candidates and must
  produce byte-for-byte the independently assembled SSZ body with one change.
  A one-trial budget includes no change. Cancellation leaves the checkpoint
  untouched. The body passes normal proposal signing, received-block transition,
  journal reopen and persisted-state checks across nine blocks.
- The final fixture requires no withdrawal for that validator in the same block
  as its credential change, then requires a later positive credit to the requested
  execution address. Two withdrawals are observed overall, versus one in the
  baseline. This is native C++ integration evidence, not an external-client
  differential test of this new selection path.
- Core, native CLI and transition runner compile successfully. Existing macOS
  spawn deprecation warning remains. Final source/binary hashes were captured
  during the final test and checked afterward. Patch applies to the recorded base
  with `git apply --unidiff-zero`.

## Still open before autonomous operation

This adds the missing candidate-selection API. `propose_current` and the validator
scheduler still use the existing no-change call. A durable bounded pending-operation
queue, admission/replay rules, restart/reorg behavior, authenticated RPC ingestion,
P2P export/inventory/relay and scheduler wiring remain necessary. Pending operations
are not persisted by this API. Byte-budget reservation is implemented, but a
near-full transaction-payload saturation scenario is not qualified here.

No public activation, new deployment, funds, complete EIP-4895 conformance or
production readiness is claimed. Voluntary-exit selection, sync contributions and
live inter-fork migration retain their separate open gates.
