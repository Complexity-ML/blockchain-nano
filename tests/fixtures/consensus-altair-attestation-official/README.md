# Altair attestation operations

All 41 minimal altair/operations/attestation/pyspec_tests cases from
ethereum/consensus-spec-tests at bc5c1a7fb2a8871aaffd4b16ee4dd9c72bb81908:
17 valid post-states and 24 invalid attestations (no post-state upstream).
The manifest records upstream paths, sizes and SHA-256 digests for original
Snappy and decoded SSZ pre, attestation and available post files.

Reference: ethereum/consensus-specs at
fcdcf42024f33c498bff74936eabea2d96049fc2, specs/altair/beacon-chain.md,
process_attestation and get_attestation_participation_flag_indices.
Minimal presets are shared with the slashing and Altair state-root fixtures.
Source/target/head weights 14/26/14 and proposer/denominator 8/64 are the
Altair specification constants.

The native operation accepts a decoded attestation and an admitted state. These
fixtures do not qualify a complete block decoder/transition, PoP establishment,
network admission or persistent fork choice.
