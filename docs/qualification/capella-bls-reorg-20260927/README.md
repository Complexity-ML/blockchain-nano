# Capella BLS queue: sibling reorg, restart and capacity — 2026-09-27

`consensus_capella_queued_bls_reorg` adds a new native C++ scenario. No production
implementation change was needed: the existing durable queue passed these checks.
The final combined scenario passed in 5.90 s. `initial-reorg.log` preserves the
preceding reorg-only variant, before adding the second withdrawal key and capacity
case. `tests.log` and `provenance.json` identify the final source/binary.

## Branch behavior

The fixture delays validator 1's queued BLS change until the final constructed
block, then admits a signed sibling without that change. Six valid native BLS
attestations switch the selected head to the sibling and back. Assertions require:

- The included operation is absent from applicable candidates and live catalog,
  while still present in the durable envelope because it is not finalized.
- Selecting the sibling restores exactly the original signed operation to the
  candidate list and catalog.
- Reconstructing the reactor from its trusted durable checkpoint preserves this
  state. Normal next-slot proposal construction includes the exact operation and
  passes the existing complete unsigned block validation without publishing it.
- Returning to the original branch removes the operation from candidates/catalog
  again, but preserves its durable archive; another reactor reopen agrees.

The two new reopen checks reconstruct the reactor on the same open ChainStore.
They test durable deserialization and branch-dependent applicability, not process
crash recovery. The new sibling deliberately omits the operation; the proposal
rebuilt after the reorg is validated but not signed/published. The fixture's one
observed withdrawal belongs to the earlier validator-0 operation. It does not
claim a second payment after this orphan/re-selection sequence; that has a separate
network evidence scope in the preceding relay report.

## Capacity behavior

A separate native reactor is initialized with a one-entry message budget. It
admits one valid request. A second request, signed by a distinct authorized
withdrawal key for validator 2, must hit the queue capacity guard, leave the
checkpoint token unchanged and retain the first operation. Repeating the first
request at full capacity is idempotent and does not rewrite the checkpoint.
This proves the configured boundary, not production throughput or large-queue
performance. No limit is increased to hide saturation.

## Remaining gates

These are synthetic local tests, without a network partition or distributed
stake. Deep/multiple reorgs, origin-state pruning/re-anchoring, concurrent admission
at capacity, write failures/crashes during admission and large inventories remain
open. EIP-4895 and production remain partial. Source patch is relative to the
recorded base and reverse-checked; source/binary hashes were checked before and
after the final test.
