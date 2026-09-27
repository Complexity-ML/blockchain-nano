# Capella pruning primitive and stable network anchor — 2026-09-27

## Operational gap found

The running Capella reactor still retains every fork-state record and eventually
hits its configured snapshot block/state-byte budget. Its former network identity
also depended on the storage anchor. Simply moving that anchor would change the
advertised network identity. This tranche implements prerequisites and a pure
candidate transformation; it does not enable live automatic pruning.

## Implementation

`CapellaForkStore::network_anchor` preserves the original trusted anchor after a
storage-anchor move. Legacy unpruned stores retain canonical v1 bytes. Pruned
stores use v2 with an explicit `networkAnchor`; both formats are validated and
restored. Reactor network identity uses the original anchor, and publication
continues to reject a changed identity. The existing live anchor-change guard is
intentionally still present pending safe queue/archive reconciliation.

`prepare_capella_pruned_store` requires a new anchor on finalized ancestry and
sufficient recent epoch context for fork choice. It retains descendants of that
anchor, the current checkpoints, and up to 256 newest-first authenticated execution
ancestor headers. It retains latest-message epoch watermarks; messages pointing
outside the retained tree move to the new anchor instead of being erased. The
chosen head must remain identical. Unfinalized/too-recent anchors, cancellation and
native deposit-vote mode are rejected. No source record or persisted history is
mutated by the function.

Native deposit voting requires pinning its referenced execution history first;
rejecting it here is a temporary unsupported path, not a completed production
solution. Persistent EVM history and signed block archives are not deleted.

## Qualification

The new local scenario builds 40 blocks with two independent execution owners,
272 votes and finality at epoch 3. Pruning its 41-record store leaves 18 records;
serialized bytes fall from 1,893,314 to 873,602. This is checkpoint size, not measured
RocksDB space reclaimed.

Assertions cover canonical v2 roundtrip, unchanged checkpoints, all available
execution-history hashes, retained vote watermarks, a dormant old vote remapped
without losing its epoch, idempotent repeated pruning, rejection of an unfinalized
anchor/native deposit-vote mode and cancellation without input mutation. A reactor
initialized over persisted execution history with the pruned store has exactly
the original network identity, including after trusted-token restart. The same
next signed block is executed against the old and restored-pruned stores; Beacon
state, execution state root and execution block hash must agree.

`initial-tests.log` records the passing legacy signed-transition regression and
first primitive run. `tests.log` records final coverage after adding dormant-vote,
idempotency and unsupported-mode checks and hardening format validation. The
source/binary hashes in `provenance.json` were captured before the final run and
verified afterward. The patch is relative to its recorded base commit.

## Remaining production work

Automatic reactor scheduling/publication, persistent archive/queue reconciliation,
native deposit-history pinning, pending-operation admission-root re-anchoring,
long-running bounded storage, adverse branches and crash recovery across pruning
are not qualified here. This API cannot yet replace the running node's growth
behavior. No production readiness, EIP closure, deployment or disk-space recovery
is claimed.

Retained final logs normalize trailing whitespace only.
