# Official Phase0 BeaconBlockBody SSZ fixtures

Source: `ethereum/consensus-spec-tests`, pinned revision
`bc5c1a7fb2a8871aaffd4b16ee4dd9c72bb81908`.
Five mainnet Phase0 `ssz_static/BeaconBlockBody/ssz_random/case_0..4`
fixtures include original roots.yaml, serialized.ssz_snappy, and unmodified
raw SSZ decompressed with libsnappy. Manifest contains exact paths, sizes and
SHA-256 digests; upstream MIT license is included. No Python or Snappy runtime
is required by the C++ tests.

The corpus exercises 20 proposer slashings, 6 attester slashings, 24 attestations,
14 deposits and 21 exits. Random SSZ fixtures validate serialization/roots, NOT
operation signatures or valid consensus state transitions.

The Phase0 mainnet and minimal presets share these body type bounds:
16 proposer slashings, 2 attester slashings, 128 attestations, 16 deposits,
16 voluntary exits and 2048 validators per committee; deposit proofs contain
33 roots. Production code takes the list bounds explicitly from its caller.
This schema must not be used for Altair or later block bodies.
