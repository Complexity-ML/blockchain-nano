# Native validator admission and activation — 2026-09-27

## Scope

The native Capella pruning fixture now has a second mode. After the original
64 synthetic genesis validators finalize and one owner prunes/reopens, a signed
execution transaction deposits the configured maximum effective balance for a
new BLS public key (index 64). No validator is injected directly into Beacon
state. The execution sender has 34 native units in this test genesis; 32 are
sent to the real deposit contract. No public funds are involved.

The native voting-period selection must consume the deposit exactly once.
Before consumption there are 64 entries; afterward there are 65. The admitted
key, withdrawal credentials, balance and effective balance must match the
deposit, and its activation epoch must initially be the far-future sentinel.
While pending, the reactor must refuse its attestation signing requests without
changing its checkpoint. A scheduled activation epoch must remain stable.

The loop continues through a bounded interval derived from the native voting
period and configured maximum seed lookahead. Once activated, the key must
receive attestation duties and produce votes accepted by both reactors. Votes
before its scheduled activation epoch fail the test. Both owners must retain
the 65-entry registry and activation epoch after database reopen. Per-block
Beacon/execution equality, cursor monotonicity and duplicate-block idempotence
remain required. The existing top-up test is rerun because the harness is shared.

## Limits

This uses two in-process local owners and a synthetic 64-validator trusted
genesis. It does not demonstrate WAN transport, bulk churn, invalid deposit
signature handling in this integrated scenario, source-chain reorg recovery,
activation under partitions or failures at admission/activation commit. Native
deposit voting does not establish EIP-6110 conformance. Sync aggregates remain
empty in this local proposer; productive sync participation is separate work.
No production-readiness claim follows from this run.

## Result

Both tests passed, 245.55 s combined. The existing-validator top-up regression
took 85.92 s; admission/activation took 159.63 s. The new deposit was consumed at
slot 80, the validator activated at epoch 17 and produced three accepted votes.
The loop reached slot 161 and finalized epoch 18. Both databases reopened with
the 65-entry registry and stable activation epoch. No runtime change was needed.
Source and binary hashes were verified unchanged after the run. Logs normalize
trailing whitespace only. Build retains an existing macOS process-helper warning.

This proves the observed end-to-end lifecycle; it does not independently derive
the exact activation scheduling epoch from a reference client or check every
churn/finality boundary. Those scheduling primitives have separate tests.
