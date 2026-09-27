# Native Capella v3 pruning/restart — 2026-09-27

The new `consensus_capella_pruning_native` scenario places the deposit-contract
code from the existing Ethereum fixture in the execution genesis and enables the
native deposit-voting profile. The trusted Beacon fixture keeps its 64 synthetic
validators; initial deposit count/cursor are zero. This is not replaying their
historical registration through the deposit contract.

The two-owner loop produces 40 signed blocks with votes/finality. It prunes to
the finalized anchor, verifies snapshot v3 and canonical roundtrip, then closes
and reopens RocksDB through the actual execution owner/reactor. It also recomputes
outer digests for deliberately corrupted snapshots: altered deposit root, missing
oldest ancestor and downgrade to v2 without pins must still be rejected.

A signed 1-unit native-asset deposit transaction is submitted after pruning. The
same following block is applied to pruned and unpruned stores. Both must produce
the same Beacon state, execution root, block hash and single-entry deposit history.
A second database reopening must retain the new deposit history. This confirms
execution/deposit recording, not subsequent consensus consumption or validator
activation/top-up by a completed deposit voting period.

Inherited checks cover cancellation, idempotent pruning, network identity and
BLOCKHASH history. Automatic scheduling, physical/sync crashes during pruning,
slashing-proof retention, long-running resource bounds and WAN behavior remain
open. Historical pin metadata still grows with ancestry. No deployment or
production-ready claim follows from this test.

Result: passed in 41.85 s. Checkpoint reduced from 41 records / 2,528,208 bytes to 18 records / 1,191,779 bytes. This is serialized checkpoint size, not measured reclaimed disk space. Source/binary hashes verified after the test; logs normalize trailing whitespace only.
