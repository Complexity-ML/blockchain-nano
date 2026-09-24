# Official block scenarios: RANDAO projection

Selected cases from `ethereum/consensus-spec-tests`, revision
`bc5c1a7fb2a8871aaffd4b16ee4dd9c72bb81908`, minimal Phase0 sanity blocks:
`empty_block_transition`, `empty_epoch_transition`, `skipped_slots`.
Original Snappy files, their unmodified raw SSZ decompressions (libsnappy 1.2.2),
metadata, hashes and upstream MIT license are retained. The C++ tests use no
Python runtime or extra decompression dependency.

The test reads the proposer key from the pre-state using the block's proposer
index, verifies the RANDAO BLS epoch signature with pre-state fork/genesis data,
applies epoch rotation, and compares every final RANDAO mix, epoch and last
block slot with the official post-state projection. There are three reveals,
192 mixes, and 15 locally generated invalid/replayed reveal cases.

It does not independently select the proposer, verify the block signature,
execute other block operations or compare complete state roots. These three
valid official scenarios must not be presented as official invalid-RANDAO tests
or full block/epoch conformance. Locally mutated signatures, fork/genesis,
proposer keys and repeated slots are separate negative checks. The shared SSZ
reader is test-only and limited to the Phase0 minimal schema.
