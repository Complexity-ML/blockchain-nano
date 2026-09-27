# Capella inventory-driven catch-up — 2026-09-27

Base revision `db5a6a3` plus accompanying changes.

Block export now includes slot and parentRoot from the same locked state as its
payload/checkpoint. These are scheduling hints; admission still verifies signed
SSZ, dependencies and execution.

The wire scenario now creates a second independent ChainStore/reactor from the
original trusted anchor. It advances its local clock, retrieves root catalogs in
two-entry pages, exports payloads, orders blocks by slot/root and sends each through
its own bounded Capella service before votes and slashings. Test-side batches are
bounded to 1000 messages/16 MiB JSON. Sixteen messages (nine blocks, six votes, one
slashing proof) converge to the source head and EVM state, with identical retained
votes/slashings and finalized checkpoint after the source's two reorgs.

The enriched test passed in 5.76 s. Its finalized checkpoint is still epoch zero:
this proves catch-up/state convergence, not new finalization. Existing scheduler
finality evidence is separate. The follower is another coordinator in the same
process, with independent storage; there are no RPC sockets or libp2p peers in this
scenario. The fetching loop is test infrastructure, not a production sync daemon.

Next work includes bounded peer synchronization, dependency recovery across
pages/pruning, network transport, and catch-up while finalized checkpoints advance.
No public activation or real funds.
