# Altair BeaconState SSZ roots

All 123 minimal-preset BeaconState static cases from
`ethereum/consensus-spec-tests@bc5c1a7fb2a8871aaffd4b16ee4dd9c72bb81908`,
under `tests/minimal/altair/ssz_static/BeaconState`.
The manifest records upstream paths, compressed and decoded file hashes, and
expected-root file hashes. Raw Snappy files are retained alongside decoded SSZ.
Seven groups are included: lengthy, max, nil, one, random, random_chaos and zero.

The minimal Altair preset is pinned separately to
`ethereum/consensus-specs@fcdcf42024f33c498bff74936eabea2d96049fc2`.
Shared Phase0 preset values come from `../consensus-slashing-official`.
Upstream license is retained in LICENSE.

This corpus qualifies schema encoding and hash_tree_root, not consensus-valid
validator lifecycle, BLS keys, transitions, fork-choice events or a live network.
