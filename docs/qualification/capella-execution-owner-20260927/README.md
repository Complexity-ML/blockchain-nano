# Capella execution owner — 2026-09-27

Base revision: `94796f0`; qualification applies to the accompanying source diff.

The durable Capella coordinator can bind a `Blockchain` owner using the shared
Beacon atomic checkpoint publication path. It publishes canonical state, tip,
finalized execution block, pool reconciliation and events after durable success.
Pool-backed body preparation snapshots pending transactions under the owner lock.
Reactor operations share the storage reactor mutex; copying is disabled.
Owner reads are closed before attachment, after detachment/invalidation, and on
uncertain checkpoint publication. Restore verifies head receipts and canonical
finalized execution ancestry before enabling reads.

## New evidence

`consensus_capella_execution_owner`: two independent owner databases in one
process, 64 synthetic validators, 40 locally assembled/signed blocks and 272
votes. Each block's transaction is submitted to both mempools. Epoch 4 justified,
epoch 3 finalized. Two owners and 64 signing journals reopened at slot 24;
production continues through slot 41. At epoch boundaries and after restart,
canonical state/tip, empty pool, finalized cache and direct JSON-RPC latest block,
finalized block and sender nonce are checked. An invalidated owner returns a
JSON-RPC error. There is no HTTP server or P2P transport in this test.

Initial gate: four suites passed in 51.72 seconds (see `ctest.log` and
`test-details.log`): new owner loop, existing signed transitions, injected WAL
sync failure, and Beacon execution startup gate. The sync failure scenario uses
a standalone store; it does not qualify bound-owner failure recovery.

## Limits

Shanghai Beacon genesis is constructed programmatically for this test. Public
JSON genesis activation remains Paris-only. No live Bellatrix-to-Capella Store
migration, scheduler, P2P, Beacon HTTP service, bound-owner reorg or bound-owner
WAL failure is claimed. The pool snapshot scans the pool; transaction trial
budgets do not bound that initial scan. Exit/BLS-change and sync-contribution
assembly, native deposit qualification and authenticated recovery remain open.
This is integration evidence, not production readiness or a deployment.

Final rebuild after making the reactor non-copyable: owner scenario and existing
Bellatrix signed execution chain both passed; see `regression.log`.
Source and test binary hashes are retained in `source-manifest.json`.
