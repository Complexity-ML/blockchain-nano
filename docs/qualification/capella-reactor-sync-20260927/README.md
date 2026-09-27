# Capella durable coordinator: uncertain WAL sync

2026-09-27, local macOS Release build, base revision `58dd284` plus the changes
identified by `sources.json`. Production code unchanged in this qualification.

## New evidence

`consensus_capella_reactor_sync_failure` injects EIO into the precise RocksDB
WAL sync while the coordinator admits a valid second signed Capella block.
The initial checkpoint and next slot tick are durable before fault injection.
The test requires an observed injected failure, then rejects five operations:
coordinator current/tick/block and storage latest/checkpoint reads.
The subprocess exits without database destructors flushing the uncertain WAL.

Two independent owned databases exercise retained and truncated WAL suffixes.
After reopening, exact canonical snapshot bytes and checkpoint sequence must
match the corresponding precomputed old/new state. Checks bind the execution
head, withdrawals, state root, block count, transaction and receipt indexes.
The lost block must leave no canonical block/transaction/receipt index.
Replaying the same signed block yields exactly the new snapshot in both cases.

## Results and reproduction

- Four suites passed in 30.22 s: signed transitions, complete participation
  finality, insufficient participation, and the new sync failure scenario.
- After adding explicit lost-index checks, the sync scenario was rebuilt and
  rerun successfully; `sync-final.log` and `sync-final-detail.log` contain that
  final result. Unchanged baseline scenarios were not rerun unnecessarily.
- Build: `cmake --build build-release --target nano-capella-transition-tests -j4`.
- New test: `ctest --test-dir build-release -R '^consensus_capella_reactor_sync_failure$' --output-on-failure`.

## Limits / remaining closure

macOS-only test interposition, not a physical power-loss or Linux/VPS test.
The test explicitly resolves two known snapshots before supplying the restart
token; this is not a production rollback authentication policy. Live network
activation, mixed Bellatrix/Capella Store migration, Blockchain owner/RPC/P2P
integration, archives/notifications and local signing remain open.
No production readiness or public network activation is claimed.
