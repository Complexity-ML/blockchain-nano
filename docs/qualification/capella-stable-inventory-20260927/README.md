# Capella stable root inventory — 2026-09-27

Base revision `dde0cc7` plus accompanying changes.

capture_inventory copies sorted candidate roots into an immutable object with
its checkpoint and kind. A caller-supplied root-byte budget bounds the count before
allocation and verifies vector capacity afterwards. Fixed object/string overhead
is additional. It does not retain EVM state, signed payloads or reactor lifetime.
Pages use binary-search cursors and enforce entry/serialized JSON byte limits.

The enriched wire test passed in 5.30 seconds. It captures four non-anchor block
roots at slot 5, saves a two-entry page, continues block production, votes and two
reorgs, then reads both original pages with the unchanged checkpoint. The live
checkpoint differs. Insufficient capture budget and unknown snapshot cursor are
rejected. The existing sixteen-message inventory/export and admission checks pass.

This resolves local page invalidation by ongoing consensus activity. It does not
pin signed payload retention: exporting a listed root later may report unavailable
after pruning, and live export reports its current checkpoint. Root ordering is
not dependency ordering. RPC handle ownership, aggregate snapshot quotas/expiry,
parent scheduling, authenticated HTTP and libp2p synchronization remain open.
No network/production activation or real funds.
