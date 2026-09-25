# Altair attester slashings

All 30 minimal altair/operations/attester_slashing/pyspec_tests cases from
ethereum/consensus-spec-tests at bc5c1a7fb2a8871aaffd4b16ee4dd9c72bb81908:
nine valid post-states and 21 invalid operations. Manifest SHA-256 hashes and
sizes cover upstream Snappy and decoded pre/attester_slashing/post SSZ files.

Reference: ethereum/consensus-specs at
fcdcf42024f33c498bff74936eabea2d96049fc2, Altair slash_validator and inherited
Phase0 process_attester_slashing. Presets/config are shared with slashing and
Altair state-root fixtures. The valid already_exited_long_ago fixture intentionally
has inconsistent lifecycle fields: its complete SSZ post-state is matched through
the isolated economics component, while the strict state operation must reject
that input. It is not counted as a successful strict transition.
This corpus does not qualify full block transitions or network finality.
