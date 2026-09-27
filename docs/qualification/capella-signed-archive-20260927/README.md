# Capella: atomic signed block archive

2026-09-27, local macOS Release, base `868dfd4` plus sources identified by
`sources.json`.

## Implementation

The durable coordinator stores each newly admitted signed block's exact wire
bytes in the same RocksDB batch as its Store snapshot and execution suffix.
Existing storage archive keys are immutable and include slot and payload digest.
Duplicate-root admission never backfills or overwrites the archive because the
fork-choice duplicate fast path does not reauthenticate the supplied signature.

`signed_block(root)` only exposes roots present in the authenticated current
Store. It validates archive slot, SSZ offsets/body/header commitments and BLS
proposer signature with the state's fork/genesis domain. Anchor, unknown and
legacy unarchived blocks return no bytes. Cancellation propagates.

## Evidence

Four suites passed in 31.63 seconds: signed transitions, full-participation
finality, half-participation non-finality, and uncertain WAL sync recovery.
Across the five baseline chains, 91 signed archives are compared byte-for-byte
after restart, including sibling branches retained through six EVM reorgs.
Malformed-signature duplicates cannot replace archived bytes or rewrite tokens.
The sync scenario requires the archive to be retained/lost with its block, and
its exact reconstruction when the block is replayed after recovery.

Nine additional negative controls (three mutations on each short scenario)
write internally checksummed archives with altered signature, body or slot;
the coordinator refuses all nine when read. The changed signed-transition suite
was rebuilt and rerun once: passed in 9.24 seconds. Baseline detail and final
negative-control detail are retained separately.

Reproduce with `cmake --build build-release --target nano-capella-transition-tests -j4`
and `ctest --test-dir build-release -R '^consensus_capella_(reactor_sync_failure|signed_transition|multiepoch_finality|insufficient_finality)$' --output-on-failure`.

## Limits

Archive verification is on retrieval, not an eager startup scan. It proves wire
commitment/signature against trusted Store data, not independent replay of the
archived transition. Old checkpoints are readable but are not backfilled.
Archive pruning/retention, vote/slashing/exit archives, notifications, signing,
Blockchain/RPC/P2P integration and live mixed-fork Store migration remain open.
No network activation, physical power-loss or Linux qualification is claimed.
