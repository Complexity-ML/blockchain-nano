# Altair get_proposer_head minimal corpus

Both official minimal scenarios at ethereum/consensus-spec-tests commit
bc5c1a7fb2a8871aaffd4b16ee4dd9c72bb81908 (v1.6.0-beta.0).
Original YAML/Snappy bytes, decoded SSZ, mechanically converted JSON and
SHA-256 manifests are preserved. Shared release references/configuration are
verified from ../consensus-altair-fork-head-official/ by the common runner.

One case retains the head, the other builds on its parent. All event and check
expectations are preserved. The exact official release oracle independently
confirms both cases with BLS active. Additional native guards exercise the
inclusive deadline, late proposing, timely head, remaining boost, strict head
weight threshold, large parent threshold, incompatible FFG, multi-slot/epoch
boundary reorg prevention and cancellation.

This optional proposer optimization consumes authenticated in-memory data.
It does not select validators, sign blocks, persist state or activate consensus.
