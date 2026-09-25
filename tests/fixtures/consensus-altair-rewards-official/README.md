# Altair rewards and penalties

All 15 minimal `epoch_processing/rewards_and_penalties/pyspec_tests` cases at
ethereum/consensus-spec-tests commit bc5c1a7fb2a8871aaffd4b16ee4dd9c72bb81908.
The manifest records upstream paths and hashes for original Snappy and decoded
SSZ pre/post states. This is isolated rewards processing, not a full epoch.
Pinned preset/configuration references are shared with the Altair inactivity
and state-root tests. Participation constants are defined in the pinned Altair
beacon-chain specification: source/target/head 14/26/14, denominator 64.
