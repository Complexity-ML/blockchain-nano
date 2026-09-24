# Phase0 registry update fixtures

All 15 scenarios from:
https://github.com/ethereum/consensus-spec-tests/tree/bc5c1a7fb2a8871aaffd4b16ee4dd9c72bb81908/tests/minimal/phase0/epoch_processing/registry_updates/pyspec_tests

Raw upstream .ssz_snappy and unmodified decompressed .ssz are retained, with
SHA-256 and byte lengths in manifest.json. Upstream MIT license included.
Fourteen scenarios have post-state; invalid_large_withdrawable_epoch must fail
by overflow without publishing any state.

The native component comparison covers all validator fields/balances, epoch and
slashing ledger fields in the test projection. The finalized checkpoint epoch
is read from the official pre-state; the component does not derive finality.
This is process_registry_updates only, not full process_epoch or state_root.
Minimal config/preset references come from consensus-slashing-official and their
pinned hashes are checked by the test.
