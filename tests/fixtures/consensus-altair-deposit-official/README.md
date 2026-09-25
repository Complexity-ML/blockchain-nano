# Altair deposit operations

All 19 minimal altair/operations/deposit/pyspec_tests cases from
ethereum/consensus-spec-tests at bc5c1a7fb2a8871aaffd4b16ee4dd9c72bb81908.
There are 17 valid post-states and two invalid operations without post-states.
The manifest records upstream paths, sizes and SHA-256 digests of the original
Snappy and decoded pre/deposit/post SSZ files.

Reference: ethereum/consensus-specs at
fcdcf42024f33c498bff74936eabea2d96049fc2, Phase0 process_deposit with Altair
add_validator_to_registry. The common deposit proof/signature rules remain
unchanged; Altair appends zero participation flags and inactivity scores.
Presets/config references are shared with the slashing and Altair state-root
fixtures. This is isolated operation qualification, not execution-layer deposit
root authentication, full block processing or network activation.
