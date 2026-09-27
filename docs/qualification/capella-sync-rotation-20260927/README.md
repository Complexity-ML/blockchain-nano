# Native Capella sync committee rotation — 2026-09-27

## Scope and oracle

New opt-in `--scheduler-sync-rotation` qualification uses the existing minimal
preset (8 slots/epoch, 8 epochs/period, 32 committee positions, 64 validator keys).
No production rules, committee period, stake cap or admission limits are changed.
It runs the actual native reactor, EVM, mempool, scheduler, BLS, RocksDB, signing
journals, recovery witness and automatic pruning, with a deterministic local clock.
It is not a distributed or wall-clock load test.

At each period boundary the current committee must equal the preceding state's
next committee. The run requires three boundaries and at least two actual changes
of composition. For every produced block after initial warm-up, all committee bits
must be set and its aggregate is independently verified with the committee keys
read from the block's state, parent root and previous-slot signing domain. Duplicate
committee pubkeys are retained in that BLS verification. The same-phase polling,
cancellation and backward-clock checks of the existing scheduler scenario remain.

The owner/reactor, witness-backed checkpoint and all 64 signing journals are reopened
at slot 127, immediately before the first nontrivial committee transition at 128.
A poll at the resumed slot repopulates ephemeral sync duties before the next block;
the test then requires full aggregate inclusion at and after the boundary. This
qualifies clean restart/re-signing of ephemeral duties, not SIGKILL/power-loss.
It also requires finality beyond the last period boundary and bounded retained
block records with automatic pruning, without increasing fixture storage limits.

Reference: [Ethereum consensus-specs v1.5.0, Altair validator assignments](https://github.com/ethereum/consensus-specs/blob/v1.5.0/specs/altair/validator.md#sync-committee).
Assignments concern the inclusion slot; signatures are produced during its previous
slot. Current/next committee fields provide the period assignments. This test does
not qualify the Ethereum sync-subnet contribution protocol.

## Initial failed test retained

The first test incorrectly required the first rotation to change composition.
The official `consensus-capella-slots-official/slots_1/pre.ssz` fixture has identical
initial current and next committees (SHA256 of each serialized committee:
`65958d612a4fe6a024e4efb1be78b0cebf8b1796dc5b94fb3e3d4038a2641934`).
The initial failure at 53.43s is retained in initial/ with source/binary provenance.
The corrected test preserves equality with the prior next committee and extends
through two subsequent composition changes; it does not weaken that condition.

## Reproduction and limits

`ctest --test-dir build-release -R '^consensus_capella_scheduled_(sync_rotation|finality)$' -V`

Each source.patch is relative to its provenance base. Hashes cover the test binary,
source and (for the final run) original fixture. Final source hashes are checked
after tests. Logs have trailing whitespace normalized only. Distributed rotation,
WAN/asymmetric delivery, partitions across rotation, large registries and production
readiness remain open. No VPS deployment, funds or public launch.


## Final result

Both CTests pass (209.76s total). Rotation scenario: 175.04s, 216 produced blocks,
1680 admitted votes, 6944 admitted sync messages (including resumed ephemeral
duties), three boundaries at slots 64/128/192, two composition changes, 215 full
32-position BLS aggregates verified, finalized epoch 25, restart at slot 127,
64 journals reopened, maximum 31 retained block records. The unchanged short
scenario passes in 34.72s (40 blocks, 1280 sync messages, finalized epoch 3).
These counts do not represent 64 independent network nodes or throughput results.
