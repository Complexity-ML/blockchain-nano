# Durable Capella BLS withdrawal-change queue — 2026-09-27

## Implementation

Signed withdrawal-key changes can be admitted through the native reactor API.
Admission authenticates the operation against the selected head and retains that
admission root with the canonical SSZ bytes. It does not mutate validator
credentials. One pending operation per validator is retained: exact duplicates
are idempotent, a different target cannot replace the first admitted operation.
The existing configured message and envelope budgets bound the queue.

The durable v3 envelope commits queue, fork state, votes and slashing proofs in
one checkpoint. Restart checks the checksum, canonical encoding, validator index,
retained admission state and BLS authorization. Older envelopes remain readable;
a queue-free checkpoint returns to the older applicable format. Existing durable
publication and fail-closed write handling remain in use.

Both ordinary body construction and the mempool builder used by `propose_current`
select queued operations. Selection filters against current head credentials,
uses the existing vote-trial budget as its change-trial bound, reserves block
bytes, and validates every addition through the complete unsigned transition.
Already included operations remain archived until finality, but are not selected
again on a head with execution credentials. Finalized execution credentials retire
the queued entry. If the admission root is removed, the operation is revalidated
against finality before re-anchoring or removal.

## Evidence

- `tests.log`: baseline signed transitions, explicit candidate selection and new
  durable queue scenario: 3/3 passed, 25.29 s. New queue checks cover invalid
  signatures, cancellation, duplicate/conflicting signed operations, unchanged
  consensus state on admission, trusted-checkpoint restart before inclusion,
  ordinary builder selection and a subsequent withdrawal to the requested address.
- `owner-builder.log`: two independent durable execution owners, mempools, signing
  journals and normal body/proposal construction; 40 blocks, 272 votes, finalized
  epoch 3, justified epoch 4, two store reopenings and 64 reopened signing journals.
  The requested withdrawal is paid and both final durable envelopes have removed
  the BLS queue. Passed in 32.65 s.
- `proposer.log`: final variant exercises `propose_current` directly for queued
  operation proposals; retains the same execution, finality, reopen and queue
  retirement assertions. Passed in 34.56 s: 40 blocks, 272 votes, finalized epoch 3.
- `provenance.json` records the base commit and source/binary hashes captured
  during that final run, then verified afterward. `source.patch` records source
  changes against the base. Builds of native CLI and transition runner succeeded.

## Scope and remaining gates

This is local C++ integration evidence, with synthetic keys and two stores in one
process. It is not a new WAN, P2P, independent-client or crash-during-admission
qualification. RPC/P2P ingress and relay for these operations remain absent.
Reorg re-selection, origin-state pruning, queue saturation and write-fault
injection for this new envelope need dedicated coverage. The scheduler shares
`propose_current`, but its live timed loop with remotely received BLS changes is
not qualified here. No public activation, real funds, complete Capella/EIP-4895
conformance or production readiness is claimed.

The prior [selection report](../capella-local-bls-changes-20260927/README.md)
retains the pinned Ethereum Capella specification reference and prior evidence.
