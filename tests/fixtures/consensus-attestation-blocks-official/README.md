# Deferred Phase0 attestation block sequences

Source:
https://github.com/ethereum/consensus-spec-tests/tree/bc5c1a7fb2a8871aaffd4b16ee4dd9c72bb81908/tests/minimal/phase0/sanity/blocks/pyspec_tests

Complete pinned `attestation` and `duplicate_attestation_same_block` sequences,
each with two blocks (slots 9 and 17). Raw SSZ-snappy, decompressed SSZ, upstream
metadata and SHA-256/lengths retained. MIT license included.

Both cross an epoch boundary. Their hashes and cross-epoch requirement are
checked, but their state transitions are DEFERRED until full epoch processing
is implemented. They are not passing block-conformance cases. A separately
constructed signed-block test qualifies current partial operation composition.
