# Slashing evidence across native Capella pruning — 2026-09-27

The new native-profile scenario adds a signed double-vote proof for validator 0
after the 40-block local chain reaches finality. The conflicting indexed
attestations share the target epoch but differ in the signed block root. Their
BLS signatures use the fork version and genesis domain from authenticated context.
The test delivers slashing evidence; it does not admit the conflicting attestation
as a normal fork-choice vote or claim that its referenced block exists.

The first attestation references the original anchor, which pruning removes from
the full block-state map. Assertions require the equivocating index to remain,
the exact signed proof archive to survive v3 checkpoint/RocksDB reopening, and
resubmission with the pair reversed to leave the checkpoint token unchanged.

The next locally assembled signed block must include the retained proof and set
validator 0's consensus `slashed` flag. The same block must produce identical
Beacon state, EVM state and block hash on pruned/unpruned stores. The signed proof
must remain archived while the slashing inclusion is not finalized. The inherited
native scenario also records a new execution deposit and reopens that history.

This is local qualification with synthetic validator keys. It does not test a
network partition, competing finalized branches, a reorganization of the slashing
inclusion, final archive retirement, or physical failure during this operation.
No production readiness or EIP closure is claimed.

Result: passed in 41.98 s. No runtime correction was required. Source/binary hashes captured before testing and verified afterward. Retained logs normalize trailing whitespace only.
