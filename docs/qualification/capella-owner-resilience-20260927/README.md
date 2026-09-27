# Capella owner reorg and uncertain-write recovery — 2026-09-27

Base revision `1b006bf`, plus accompanying test changes. No production algorithm
change or public fork activation in this tranche.

## New scenarios

- `consensus_capella_execution_owner_reorg`: a bound Blockchain owner follows two
  competing signed Capella blocks, one with a transaction and one empty. Six
  authenticated committee votes force selection of the empty branch and then
  restoration of the original branch. After each vote, owner tip/state match
  the reference Store; the orphan transaction returns to the pool, canonical
  transaction/receipt indexes disappear, and the direct receipt RPC returns null.
  When the original branch wins again, the pool drops the transaction and the
  receipt is visible again. Two forced branch reversals are checked.
- `consensus_capella_execution_owner_sync` (macOS): inject EIO into fsync /
  F_FULLFSYNC for the owned RocksDB WAL during a bound coordinator block commit.
  Eight direct operations refuse access, and an already-created RPC returns an
  error. Exit the child without destructors. Test both retention of the appended
  WAL suffix and loss of that suffix by truncation to its recorded pre-write
  boundary. Reopen the owner and coordinator, verify exactly old or new Store,
  paired execution state, canonical indexes and signed-block archive, then replay
  the block. Owner state/tip and latest-block RPC agree with the completed Store.

## Limits

Synthetic local fixtures, no P2P or HTTP server. Injected synchronization error
and controlled WAL suffix loss are not a physical power-cut test. The fixture
knows both restart outcomes and supplies the matching checkpoint token; production
still needs independently authenticated recovery. Pool persistence across process
restart is not asserted. Genesis is programmatic Shanghai Beacon; public startup
remains Paris. Live Store migration, runtime scheduling and Capella transport remain
open. This evidence does not establish full production readiness.

## Results

Four suites passed in 28.78 seconds: both new scenarios, existing standalone
WAL failure, and signed transitions. See `ctest.log` and `test-details.log`.
