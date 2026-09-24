# Phase0 attester slashing block fixtures

Four single-block scenarios pinned to:
https://github.com/ethereum/consensus-spec-tests/tree/bc5c1a7fb2a8871aaffd4b16ee4dd9c72bb81908/tests/minimal/phase0/sanity/blocks/pyspec_tests

attester_slashing, multiple_attester_slashings_no_overlap,
multiple_attester_slashings_partial_overlap, invalid_duplicate_attester_slashing_same_block.
Raw SSZ-snappy, decompressed SSZ, upstream metadata and hashes retained; MIT license.

Three valid registry/RANDAO projections match official post-state (192 validator
records, five proofs). Duplicate proof passes block authentication then fails
operations without publishing the earlier sanction. As with preceding block
projection corpora, parent commitments are checked against official post-history
for valid blocks, not independently derived by a full slot/state_root transition.
