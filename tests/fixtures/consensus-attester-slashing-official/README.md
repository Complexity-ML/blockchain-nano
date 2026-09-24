# Phase0 indexed attester slashing fixtures

All 30 scenarios from:
https://github.com/ethereum/consensus-spec-tests/tree/bc5c1a7fb2a8871aaffd4b16ee4dd9c72bb81908/tests/minimal/phase0/operations/attester_slashing/pyspec_tests

Raw upstream SSZ-snappy and unmodified decompressed SSZ, hashes/lengths in manifest.
MIT license retained. Nine valid isolated post-states, 21 invalid operations.
Funding/PoP is an authenticated-registry precondition; both aggregate signatures
and indexed proof conditions are verified here without importing expected weights.

`already_exited_long_ago` deliberately sets withdrawal epoch 2 for entries whose
exit epochs are 5 or 6. The isolated operation component matches its official
post-state; the checked chain application rejects that inconsistent lifecycle.
The test asserts both outcomes explicitly. The other eight valid scenarios and
21 invalid cases are compared through both component and checked entry points.
No global chronology bypass is exposed as an option on block processing.

The full comparison includes all validator/balance fields and the slashing ledger.
This is not a full BeaconState, state_root or live-consensus qualification.
