# Phase0 justification/finalization fixtures

All ten cases from:
https://github.com/ethereum/consensus-spec-tests/tree/bc5c1a7fb2a8871aaffd4b16ee4dd9c72bb81908/tests/minimal/phase0/epoch_processing/justification_and_finalization/pyspec_tests

Raw upstream SSZ-snappy and unmodified decompressed SSZ; SHA-256/lengths in
manifest.json. Upstream MIT license retained. All ten have valid post-state.

The native test reads registry, RANDAO, block-root history, checkpoints/bits and
pending-attestation aggregation bitlists from pre-state. Production component
reconstructs committee members and effective weights, then compares all output
checkpoints/bits with official post-state. No precomputed participation totals
are imported. Includes poor support, mismatched target and exited-validator cases.

Pending attestations are authenticated BeaconState records, not signed network
messages: their signatures/source/inclusion were checked before storage in the
protocol. This test does not qualify that admission step, full process_epoch,
fork choice, SSZ state_root, persistence or network finality.
