# Altair SyncAggregate operations

All 24 minimal altair/operations/sync_aggregate/pyspec_tests cases from
ethereum/consensus-spec-tests at bc5c1a7fb2a8871aaffd4b16ee4dd9c72bb81908:
16 valid post-states and 8 invalid aggregates (no post-state upstream).
The manifest records upstream paths, sizes and SHA-256 digests for original
Snappy and decoded SSZ pre, operation and available post files.

Reference algorithm: ethereum/consensus-specs at
fcdcf42024f33c498bff74936eabea2d96049fc2, specs/altair/beacon-chain.md,
process_sync_aggregate. Minimal presets are shared with the slashing and Altair
state-root fixture directories. Weights 2/8/64 are Altair specification constants.

This tests the isolated operation on an admitted state, not complete block
processing, committee admission, PoPs, network finality or durable replay safety.
