# Capella structural synchronization ordering — 2026-09-27

Base revision `d64516d` plus accompanying changes.

order_capella_sync_blocks is a core C++ primitive taking a bounded batch, trusted
Capella rules and known local roots. It checks aggregate payload vector capacity
and count, canonical signed-block offsets and complete body SSZ commitments, then
recomputes advertised block roots. Duplicate roots and missing parents are refused.
Slot/root ordering returns indexes without copying payloads; parents within the
batch must have earlier slots. Metadata slot/parent claims from the export JSON
are not used to schedule. Metadata allocations are bounded by batch count.

This is structural scheduling, not signature/state validation. BLS, proposer,
clock, EVM and fork-choice checks remain in normal reactor admission. Known roots
must come from authenticated local state, not a peer's claims.

The wire catch-up scenario now uses this core primitive instead of its ad hoc
JSON metadata sort. Four new negative cases cover wrong claimed root, duplicate,
missing anchor and insufficient payload budget. Sixteen-message catch-up still
converges to source head/state/evidence. Test passed in 5.80 s.

The production network fetch/retry loop and recovery across missing/pruned pages
remain open. The helper rejects an incomplete batch rather than fetching missing
parents. No sockets, new finalized epoch, public activation or funds in this test.
