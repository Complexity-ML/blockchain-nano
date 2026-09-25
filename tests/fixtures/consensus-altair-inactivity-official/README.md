# Altair inactivity updates

All 21 minimal `epoch_processing/inactivity_updates/pyspec_tests` cases at
ethereum/consensus-spec-tests commit bc5c1a7fb2a8871aaffd4b16ee4dd9c72bb81908.
Manifest paths and hashes retain both original Snappy and decoded SSZ files.
Rules use the pinned minimal Phase0/config files in consensus-slashing-official
and the Altair preset in consensus-altair-state-roots-official.

These are isolated component tests. `randomized_state` and
`randomized_state_leaking` contain validators with activation_epoch=UINT64_MAX
and finite exit_epoch. They do not establish valid genesis/state admission.
