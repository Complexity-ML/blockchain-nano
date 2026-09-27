# Capella: local body assembly from authenticated state

2026-09-27, macOS Release, base `a14a83d`; source hashes in `sources.json`.

## Implementation

`prepare_local_body` derives proposer/parent, timestamp, fee, RANDAO input and
withdrawals from the current Store. A shared authenticated SSZ registry/cursor
reader serves both withdrawal execution and construction. Shanghai execution
selects bounded candidate transactions by sender nonce; sender identity is
recovered internally. Withdrawals remain executed after transactions.

Caller-provided transaction and operation trial budgets bound selection. The
assembler reserves consensus-operation capacity before packing transactions,
then selects admissible retained slashing evidence and gossip attestations.
Recently included attestations on the chosen branch are excluded using verified
signed block archives. Each tentative body passes the full unsigned Capella
transition. Configured native deposits use the existing mandatory-deposit APIs.
The resulting body can be signed by the local journal-bound proposal API.

## Evidence

Four integration suites passed in 53.64 s. Twenty-four bodies across the three
short chains exactly match independent construction and are signed/admitted.
Five additional bodies (one per baseline scenario) exercise out-of-order nonce
candidates, a transaction with zero fee cap, local signing/admission, and reopen.
Both valid transactions execute in nonce order; the underpriced candidate is
excluded. Three pending slashing proofs are included, and their offenders are
marked slashed in the resulting consensus registry. Cancellation leaves the
checkpoint unchanged. The WAL scenario repeats the short-chain coverage.

Three directly affected withdrawal suites passed in 0.90 s:
`consensus_capella_withdrawal_execution`, `ethereum_official_capella_payload`,
`consensus_capella_payload_paired_state`. The official payload suite still checks
SSZ withdrawal processing against its pinned fixture outputs after factoring
the authenticated reader.

Build `nano-capella-transition-tests`, `nano-capella-execution-tests`,
`nano-capella-payload-tests`, `nano-capella-withdrawal-tests`. Test expressions
are recorded in the saved CTest logs/details.

## Limits / remaining work

Transaction candidates are supplied explicitly; live Blockchain mempool ownership
and network scheduling are not wired. Native-deposit assembly is guarded but not
independently qualified by this run. Voluntary exits and BLS credential changes
are not selected here; sync participation is empty (with normal consensus
penalties), not a live sync-contribution service. Automatic vote selection exists
but end-to-end finality produced solely by this assembler remains to qualify.
Proof retirement, complete scheduling, owner/RPC/P2P integration and live fork
migration remain open. No public activation or production claim.
