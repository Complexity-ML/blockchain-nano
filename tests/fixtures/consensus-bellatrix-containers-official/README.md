# Official Bellatrix SSZ containers

Source: ethereum/consensus-spec-tests, pinned in `manifest.json`, minimal preset,
`bellatrix/ssz_static/{ExecutionPayloadHeader,BeaconState,BeaconBlockBody}/ssz_random`,
cases 0 through 7 for each type. These are 24 selected format vectors, not the
complete Bellatrix corpus or valid signed block transitions.

The manifest records the original roots, Snappy bytes, decompressed SSZ, sizes
and SHA-256 digests. The three minimal presets are pinned to consensus-specs
v1.6.0-beta.0. C++ tests consume decompressed SSZ directly; importing the vectors
does not introduce a Python or Snappy runtime dependency.
