# Altair reorg official events

All 8 minimal scenarios from ethereum/consensus-spec-tests at
bc5c1a7fb2a8871aaffd4b16ee4dd9c72bb81908 (v1.6.0-beta.0).
Original YAML and Snappy bytes are preserved; decoded SSZ and mechanically
converted JSON events are included. manifest.json records paths, sizes and SHA-256.
Shared pinned fork-choice references and historical time/boost parameters are
in ../consensus-altair-fork-head-official/. The native runner verifies all hashes
and published event validity, head and checkpoint expectations with BLS active.
A received block also delivers its authenticated embedded votes and slashings.

This corpus qualifies an in-memory Store only. It does not establish persistent
recovery, P2P/EVM integration, mainnet-scale performance or production readiness.
