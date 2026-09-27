# Capella admission receipts — 2026-09-27

Base revision `9e41a18` plus accompanying changes.

The runtime admission service exposes submit/get/list/rotate operations backed by
AltairReceipts and the existing ordered Capella worker. It does not create a second
consensus queue. Envelope validation precedes ledger admission. Receipt capacity
uses the configured job limit; completed entries require explicit retirement.

The configured-runtime test passes in 21.55 seconds, producing 16 blocks with
64 synthetic keys and witnessed restart. Six additional checks cover committed
outcome retrieval, identical retransmission, conflicting payload reuse, retired
sequence replay, cancellation before submission and stale-session rejection after
rotation. All receipt operations leave the archived duplicate block checkpoint
unchanged. Existing invalid configuration and stopped-worker checks still pass.
Only the changed runtime test was rerun; no broad compatibility claim is made.

Receipts are process-local, not durable transaction certificates. Losing a session
means unknown outcome, not rejection; callers must reconcile with durable state.
This exposes C++ entry points for private RPC integration, not reachable HTTP RPC.
HTTP authentication/dispatch, inventory/export/synchronization and libp2p wiring
remain open. No funds, public activation or distributed network test.
