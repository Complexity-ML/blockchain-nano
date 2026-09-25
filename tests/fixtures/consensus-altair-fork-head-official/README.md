# Altair get_head event sequences

All ten minimal altair/fork_choice/get_head/pyspec_tests scenarios at
ethereum/consensus-spec-tests commit bc5c1a7fb2a8871aaffd4b16ee4dd9c72bb81908
(release v1.6.0-beta.0). Original YAML/Snappy files are preserved alongside decoded
SSZ and JSON event lists. The manifest records paths, sizes and SHA-256 hashes.
steps.json is a mechanical PyYAML 6.0.3 safe_load conversion of steps.yaml.

Reference files distinguish consensus-specs 34ed95a58164d22b200df7ef4fb6dc1301599474
(the release tag) from fcdcf42024f33c498bff74936eabea2d96049fc2 (newer reference).
The latter adds the shuffling-dependent-root proposer-boost gate. Both profiles
are explicit in the native adapter; this historical corpus selects release boost
and seconds-to-milliseconds conversion with the release's 6-second slots and
three intervals per slot.

All ten scenarios now pass: 109 block events, 104 ticks, 19 standalone votes,
one standalone slashing and 1081 checked fields. Per the official format and
generator, a block event also delivers its embedded attestations (is_from_block)
and attester slashings. An initial replay omitted these events and selected the
wrong head in voting_source_within_two_epoch. The failing campaign is preserved;
fixture expectations are unchanged. The official Python reference at the pinned
release was independently built and replayed with BLS enabled: zero mismatches.

The native reception adapter authenticates a complete block before processing
embedded messages; gossip has no caller-controlled verified-block exception.
Old gossip rejection, old embedded-vote acceptance, bad block signature rejection
and cancellation are tested separately. This qualifies the get_head corpus only,
not all fork_choice groups, persistent recovery or distributed consensus.
