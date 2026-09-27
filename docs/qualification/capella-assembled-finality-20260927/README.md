# Capella: finality from the complete local production loop

2026-09-27, local macOS Release. Base `31b292d`; test/config hashes in
`sources.json`. Production implementation unchanged in this qualification.

## Scenario

Start from a trusted, fixture-derived Capella anchor at slot 1 with 64 synthetic
validator identities. Two independent durable coordinators, each with its own
RocksDB, receive identical tick/block/vote events through direct local calls.
From slots 2 through 41, every block is assembled from the current Store and a
signed execution transaction, with local RANDAO and proposal signatures. All
attestations are derived and signed by the local duty/journal APIs. No fixture
attestation list is injected into the produced blocks.

The test compares complete canonical Store snapshots, checkpoint tokens, vote
archives, canonical execution heads and state roots at epoch boundaries.
Finalized epoch must never regress. At slot 24 both Stores and all journals then
in use are closed/reopened with retained commitments, followed by continued
production through slot 41. Final sender nonce verifies all 40 new execution
transactions plus the anchor transaction.

## Results

| Participation | Produced blocks | Signed votes | Justified epoch | Finalized epoch | Journals reopened |
| --- | ---: | ---: | ---: | ---: | ---: |
| All 64 identities | 40 | 272 | 4 | 3 | 64 |
| 32 even-index identities | 40 | 133 | 0 | 0 | 44 |

The insufficient-participation case still produces blocks but cannot finalize.
Its 44 reopened journals include proposer keys outside the participating half.
The two coordinator Stores remain equal, including after their two reopens.

Both new suites initially passed in 52.10 seconds. The report initially counted
all journals used by the end, rather than those reopened at slot 24. Only that
reporting counter was corrected; the affected half-participation scenario was
rebuilt/rerun and passed in 21.38 seconds. Its initial count 47 is superseded by
44 in `half-final-detail.log`. The full-participation count remains 64.

Build `nano-capella-transition-tests`; run
`ctest --test-dir build-release -R '^consensus_capella_local_assembly_' --output-on-failure`.

## Scope and remaining closure

This closes local assembled-chain finality/reopen coverage, not public network
readiness. These are two in-process coordinators, not separate network nodes;
there is no P2P transport, latency, partition, clock skew or distributed signer.
Normal close/reopen is not SIGKILL or physical power loss. Mainnet activation,
initial live fork crossing, production scheduling, mempool owner integration,
missing validator operation queues and sync contributions remain open.
