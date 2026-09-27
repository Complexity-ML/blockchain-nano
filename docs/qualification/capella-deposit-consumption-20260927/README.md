# Native deposit consumption after pruning and restart — 2026-09-27

The new scenario extends the existing native v3 test. A real signed transaction
records a one-unit native-asset deposit for the public key of existing validator 2
after pruning. Its execution deposit history is first checked across database
restart. Both nodes then run with automatic pruning through the next native
voting-period majority and one further epoch.

Every following block must produce identical Beacon state, execution hash and EVM
root on both owners. The deposit cursor must be monotone and at most one, with
exactly one observed transition from zero to one. At that transition, the existing
validator must have exactly its slot-advanced pre-block balance minus advertised
withdrawals, plus the deposit, minus the empty-sync penalty for each occurrence
of its public key in the current sync committee. The scenario explicitly requires
that validator 2 is not the proposer (excluding inclusion rewards), and verifies
that the sync participation bits are empty. Registry size remains 64.

The penalty is independently computed in the test from the active effective
balance, integer square root and configured Altair reward constants; it does not
call the production sync-transition function. See the [Altair consensus
specification](https://ethereum.github.io/consensus-specs/specs/altair/beacon-chain/#sync-aggregate-processing).
This native deposit-vote mechanism is not itself Ethereum EIP-6110 conformance.

Replaying each accepted block must leave the checkpoint token unchanged. After
additional blocks, both RocksDB owners reopen with the consumed cursor still one.
The expected consumption window is derived from the configured native voting
period, not an arbitrarily fixed wait or altered production threshold.

This tests an existing-validator top-up, not a newly funded validator's admission
and activation. The 64 original validators are synthetic trusted genesis entries.
It does not qualify a source-chain bridge, deposit reorganization, external Eth1
consensus or WAN transport. Physical/sync failures at deposit consumption remain
separate work. No production-readiness claim follows from this local scenario.

## Initial failure and correction

The first run failed after 77.43 s because the test ignored empty-sync penalties.
A diagnostic rerun reproduced the failure at slot 80: pre-block balance
32,007,731,162 gwei, post-block balance 33,007,719,982 gwei, no withdrawals,
registry size 64. The difference from a gross one-unit top-up is 11,180 gwei.
The corrected assertion accounts for these penalties exactly; no production
consensus rule was changed or validation bypassed. Both failure logs are retained.
The diagnostic run's source differs only by logging from the initial version;
`initial-provenance.json` describes the initial failed run, not the diagnostic binary.

## Final result

Passed in 87.06 s. The deposit cursor advanced once at slot 80 in the configured
32-slot voting period and remained one through slot 89. One sync committee
position debited 11,180 gwei; the exact expected and observed balances both equal
33,007,719,982 gwei. Both owners reopened with the consumed cursor. Per-block
Beacon/EVM equality and idempotent block replay passed. Source/binary hashes were
verified unchanged after the run. Logs normalize trailing whitespace only.
The build retains an existing macOS process-helper deprecation warning.
