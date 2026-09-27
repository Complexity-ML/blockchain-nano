# Capella: authenticated local attestation signing

2026-09-27, local macOS Release. Base `1e8cde7`, source hashes in `sources.json`.

## Implementation

Capella duty derivation now selects the current committee from authenticated
Store registry/RANDAO and derives head, source/target checkpoints, membership
bit and fork/genesis domain. It verifies paired execution records and excludes
slashed or equivocating validators. Missing retained epoch boundary yields no
duty. The bulk duty API traverses the state once for all current assignments.

The coordinator's `sign_attestation` accepts validator identity, expected slot,
checkpoint token and an independent clock, not caller-constructed vote data.
Under its mutex it rechecks disk commitment and binds the signing journal to the
chain, execution genesis, validator genesis, public key and epoch length. The
journal must use separate storage. Existing anti-slashing reservation is durable
before invoking the provider. Monotonic same-slot checks surround provider work
and verification; expiration does not erase the reservation.

## Evidence

Twelve local attestations (eight/four in full/half-participation scenarios) match
expected data, membership and BLS signature. Twelve reopened journals permit the
identical message with unchanged reservation, and reject conflicting messages
before provider invocation. Twelve stale-token and unassigned-key requests are
rejected; future duties and equivocators are excluded. Two wrong-chain journals
are refused and two mid-provider expirations retain conflict protection.

The initial four-suite run passed signed transitions and WAL recovery but stopped
the two finality scenarios on an incorrect test expectation that a reopened
journal caches signatures. Inspection showed the journal caches reservations,
not signature bytes; signing the identical message again is intentionally allowed.
Only that test expectation changed. Both affected suites then passed in 27.22 s.
Initial and corrected evidence are retained. No production defect was inferred
from that test mistake.

Build `nano-capella-transition-tests`; run CTest expression
`^consensus_capella_(reactor_sync_failure|signed_transition|multiepoch_finality|insufficient_finality)$`.

## Limits

Synthetic local validator keys and trusted fixture anchors. This does not activate
network validators or provide a full local validator scheduler. Capella RANDAO,
proposal construction/signing, gossip publication, journal operational custody,
owner/RPC/P2P integration and live fork migration remain open. Large-registry
signing throughput and distributed clock behavior are not qualified here.
