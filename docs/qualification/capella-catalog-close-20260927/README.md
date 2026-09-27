# Capella catalog close recovery — 2026-09-27

## Change and evidence

A close whose acknowledgement is lost can now be repeated safely. The registry
validates the principal and handle, rejects attempts against a live foreign
handle, and treats an already absent or expired handle as closed. It allocates
no tombstones and does not extend snapshot lifetimes. A successful close means
that the handle is absent; it does not prove which request removed it.

The relay retries close exactly once when the transport reports an unknown
outcome (`null`). It never retries open or consensus delivery. An explicit
negative response stops the attempt. If the callback never arrives, the existing
overall deadline still ends the relay with failure. Cancellation cleanup remains
best effort. Older cores that reject repeated closes can still cause a failed
relay result; deploy matching core and daemon builds.

- `relay.log`: 19 deterministic mocked transport scenarios, including four new
  cases: applied close/lost reply, lost request, repeated unknown outcome and
  explicit rejection. Verify one completion, bounded retry, one release and no
  additional consensus delivery. These ACKs are mocks, not consensus evidence.
- `wire.log`: real Capella reactor/registry test, including repeated close,
  principal isolation, malformed/anonymous close rejection, expired close,
  aggregate snapshot quota and capacity reclamation (6.88 s).
- `http.log`: real loopback HTTP RPC configured runtime, including two closes
  of the same handle and rejection of a subsequent page read; 16 blocks,
  restart checks and 64 synthetic validator keys. The historical summary's
  `runtimeCatalogChecks` field remains a fixed legacy count, not a coverage count.
- Core, transition runner, relay runner and `nano-p2p` compiled successfully.
  The macOS build retains the existing deprecated `posix_spawn` warning.

The manifest records modified source and tested binary hashes captured after
start of the tests. `source.patch` applies to the recorded base with
`git apply --unidiff-zero`. This is a local targeted qualification, not a fresh
Docker image, distributed partition rerun, VPS rollout or production approval.

## Open blocker: lost open acknowledgement

Open still creates a random server-side handle. If its response is permanently
lost, the relay cannot address that handle and quota is retained until TTL.
Late responses that do arrive are cleaned up, but that does not solve a lost
response. No claim of complete catalog recovery is made here.

Next protocol work needs an authenticated client request identifier and bounded
idempotency/cancellation state. Tests must cover cancel-before-open reordering,
duplicate open, conflicting kind, expiry, restart, owner isolation and quota
exhaustion without resurrecting a closed snapshot. Blindly retrying today's open
would allocate additional snapshots and is deliberately not enabled.
