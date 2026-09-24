# Official Phase0 slashing component vectors

All five cases in `tests/minimal/phase0/epoch_processing/slashings/pyspec_tests`
from ethereum/consensus-spec-tests at
`bc5c1a7fb2a8871aaffd4b16ee4dd9c72bb81908` are retained here, with its MIT license.
The manifest records upstream paths and SHA-256 hashes. Original `pre`/`post`
Snappy files are retained; their `.ssz` counterparts are raw decompressions
with libsnappy 1.2.2, without field rewriting. No Python runtime is used by tests.

The C++ test reads the Phase0 minimal SSZ layout directly and compares the
slashing state projection and every balance against the unchanged official
post-state. Minimal preset/config files have their own pinned consensus-specs
revision and hashes in the manifest. Test constants for field offsets are SSZ
schema dimensions, not network validator limits or Nano economic defaults.

These are isolated `process_slashings` vectors, not full epoch transitions.
Several pre-states set a finite withdrawable epoch while leaving exit_epoch at
FAR_FUTURE_EPOCH to isolate the penalty calculation. The pure accounting helper
accepts such component inputs; the complete Nano slashing epoch wrapper retains
its lifecycle checks and rejects them. The fixtures do not qualify admission,
BLS authentication, epoch rotation, state roots or a live consensus network.
`pre_epoch` and `post_epoch` are intentionally outside this selection.
