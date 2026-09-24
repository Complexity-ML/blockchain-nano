# Official blocks containing proposer slashings

Selected minimal Phase0 sanity/blocks cases, ethereum/consensus-spec-tests
revision `bc5c1a7fb2a8871aaffd4b16ee4dd9c72bb81908`:
- proposer_slashing
- proposer_self_slashing
- multiple_different_proposer_slashings_same_block
- invalid_duplicate_proposer_slashings_same_block
- invalid_similar_proposer_slashings_same_block

Original Snappy, raw decompressed SSZ, one-block meta.yaml, hashes and MIT license
are retained. Three valid cases have official post-states; two invalid cases
have none. The runner checks signatures, body root, selected proposer, RANDAO and
ordered proposer slashing transitions, comparing registry/RANDAO projections
against official post-states. Invalid cases must pass envelope authentication
and fail operation processing. Source states remain unchanged.

No complete BeaconState root/transition is claimed. In valid cases the expected
parent is checked against post-state block_roots; invalid-case parent context
comes from the fixture block so rejection exercises the operation gate. This
selected corpus has no epoch boundary or other operation kinds.
