# Phase0 registry block operation fixtures

Source: https://github.com/ethereum/consensus-spec-tests/tree/bc5c1a7fb2a8871aaffd4b16ee4dd9c72bb81908/tests/minimal/phase0/sanity/blocks/pyspec_tests

Eight named scenarios with raw SSZ-snappy, unmodified decompressed SSZ and
upstream metadata; SHA-256/size in manifest.json. Upstream MIT license retained.

The native test processes six single-block scenarios: deposit_in_block,
deposit_top_up, invalid_duplicate_deposit_same_block,
invalid_duplicate_validator_exit_same_block, slash_and_exit_diff_index,
slash_and_exit_same_index. Three valid final registry/RANDAO projections match
official post-states; three invalid blocks pass authentication then fail operations.

Two complete two-block sequences are retained but explicitly deferred:
voluntary_exit and multiple_different_validator_exits_same_block. Their second
blocks cross epoch 64 to 65. No full epoch transition is implemented here, so
they are NOT counted as successes. Completing the epoch pipeline must unlock them.

This corpus does not prove full BeaconState/state_root, Eth1 voting or live
consensus. Parent roots for valid blocks are checked against official post-state
history; invalid-block contexts deliberately accept the supplied parent to
isolate operation rejection. No claim of independent parent/state derivation.
