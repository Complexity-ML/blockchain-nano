# Complete Altair signed block sequences

All 52 minimal altair/sanity/blocks/pyspec_tests cases from
ethereum/consensus-spec-tests at bc5c1a7fb2a8871aaffd4b16ee4dd9c72bb81908.
37 sequences have a valid post-state; 15 must reject a block. Manifest hashes
and byte sizes cover original Snappy, decoded SSZ, and block-count metadata.

Reference: ethereum/consensus-specs at
fcdcf42024f33c498bff74936eabea2d96049fc2, inherited Phase0 state_transition and
Altair beacon-chain.md. Minimal presets/config are shared with the slashing and
Altair state-root fixtures.

These tests compare complete final SSZ states and verify every accepted block's
advertised state root. They exercise slots/epochs, mixed operations, slashing,
Eth1 voting, deposits/exits, participation/inactivity and sync aggregates. They
do not qualify fork-choice events, snapshot admission, persistence, distributed
network behavior or a transition between forks.
