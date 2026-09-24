# Phase0 attestation admission fixtures

All 41 scenarios from:
https://github.com/ethereum/consensus-spec-tests/tree/bc5c1a7fb2a8871aaffd4b16ee4dd9c72bb81908/tests/minimal/phase0/operations/attestation/pyspec_tests

Raw upstream SSZ-snappy, unmodified decompressed SSZ, SHA-256/lengths in manifest;
upstream MIT license retained. Seventeen valid post-states, 24 invalid operations.
Tests compare full projected pending lists, including inclusion_delay and proposer_index.
BLS verification is enabled, with epoch fork/genesis domains and committee keys
derived from the authenticated fixture registry/RANDAO.

Registry funding/PoP is a precondition established before these operations;
these fixtures do not replay all genesis/deposit admission. Incorrect head/target
roots can be valid inclusions under Phase0, unlike incorrect source/signatures.
No full epoch, state_root, durable block commit or live network claim.
