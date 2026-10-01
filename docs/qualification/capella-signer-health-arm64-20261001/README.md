# Capella signer health — Docker ARM64, 2026-10-01

Implemented: atomic nonblocking observation of the signer's latched failure,
`validatorHealthy` on authenticated private `nano_capellaStatus` (null when no
signer configured), and one fixed `capella-validator-unavailable` event through
the daemon's shared monitor entry point. A signer failure does not request node
shutdown, retry signing, reset a reservation or stop consensus admission.
Private provider failure strings do not appear in this RPC or alert.
Telemetry exceptions are caught by the daemon; child-exit handling stays active.
The earlier misleading scheduler comment was corrected against existing guards.

Docker reuses the ARM64 builder `6116669430b02b7e8012af8c8bd515a5a7dcd1dd2034649f9b45ae5b9e70a3a4`
and `nano-linux-sync-build`, read-only source, network none, nofile 65536.
Command: build `nano-capella-transition-tests nano` with -j2, then
`--signer-health` and `--managed-cli /build/nano`. Complete command exit 0.
Exact source hashes/diff and new header are retained. v1 passed too; final v2
added the best-effort telemetry exception boundary and passed both tests again.
No runtime image was packaged; volume revision marker is not updated.

NEW coverage: two real-worker faults, first RANDAO provider call before proposal
reservation and second provider call after durable proposal reservation. Tests
qualify failed signer/healthy clock, normal cancellation preserving health and
pins, authenticated HTTP status true→false, wrong-token rejection, Origin HTTP
403 rejection, private failure redaction, and exactly one alert via the SAME
entry point used by node.cpp. The provider is switched explicitly after stopping
the failed worker; same journals/witnesses prohibit a reserved-slot retry and
permit legitimate slot-3 admission. All 64 primary/witness histories reopen in
each case against independently retained exact commitments. Unconfigured signer
reports null. No automatic provider recovery is claimed.

Managed CLI regression: three starts with synthetic stub P2P child, two negative
configurations, retained witnesses, exclusion of validator key environment from
the child, and child failure still stopping the daemon. Real libp2p is NOT covered
by this CLI scenario. Alert entry-point qualification is in-process with a real
failed signer; a daemon process emitting this alert during a real provider fault,
external alert delivery, hung providers and memory-fault logging injection are
not qualified. Healthy means no latched signer failure, not guaranteed liveness.
Native Mac and Docker amd64 remain to qualify this revision. WAN N stays failed,
and this supervision change makes no WAN-performance or production claim.
