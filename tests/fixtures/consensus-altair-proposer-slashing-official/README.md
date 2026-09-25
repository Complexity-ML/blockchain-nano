# Altair proposer slashings

All 15 minimal altair/operations/proposer_slashing/pyspec_tests cases from
ethereum/consensus-spec-tests at bc5c1a7fb2a8871aaffd4b16ee4dd9c72bb81908:
three valid post-states and 12 invalid operations. Manifest SHA-256 hashes and
sizes cover upstream Snappy and decoded pre/proposer_slashing/post SSZ files.

Reference: ethereum/consensus-specs at
fcdcf42024f33c498bff74936eabea2d96049fc2, Altair slash_validator and inherited
Phase0 process_proposer_slashing. Presets/config are shared with slashing and
Altair state-root fixtures. The invalid_proposer_is_withdrawn fixture has a
synthetic inconsistent lifecycle and is rejected by strict registry validation.
This corpus does not qualify full block transitions or network finality.
