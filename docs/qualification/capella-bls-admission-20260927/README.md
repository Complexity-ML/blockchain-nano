# Capella BLS change admission — 2026-09-27

## Change

The Nano wire envelope accepts `bls_to_execution_change` only with the explicit
Capella profile capability and full consensus identity. Both peer hex frames and
compact private IPC require exactly 172 decoded SSZ bytes. Existing frame, peer,
worker, receipt and durable-queue budgets remain effective. Altair/Bellatrix
profiles default to rejecting this new message. The appended enum value preserves
existing receipt-kind values.

The Capella ordered admission worker calls the authenticated durable reactor API.
The network daemon enables the capability in Capella mode, and its IPC dispatcher
preserves the kind and complete bytes. Existing `nano_capellaSubmit` supports it
through the private token/session-protected route. No new public RPC is opened.
Signature validation remains withdrawal-key authorization, not validator signing
key possession. Success is reported only after durable admission.

## Evidence

- `wire.log`: original Capella wire-admission regression and new queued-change
  scenario both pass (11.25 s total). Invalid signature rejected; valid change
  passes the ordered worker, survives restart, is included by the ordinary
  builder, and enables a later withdrawal. Fixed-length rejection, compact
  decoding, wrong-session rejection and duplicate receipt idempotency are checked.
- `dispatch.log`: 9 dispatcher scenarios pass (0.31 s), including new BLS payload
  preservation, rejection of 171-byte input and refusal under a legacy profile.
  The RPC client in this dispatcher test is simulated, as the log explicitly says.
- `rpc.log`: configured native runtime scenario passes (17.17 s). After its
  existing 16-block/runtime-restart coverage, a real loopback HTTP server receives
  the compact BLS request. Wrong access token returns -32001 without publication;
  authorized request completes as accepted, increments the checkpoint sequence
  once and exposes one applicable pending change. All keys are synthetic.
  The legacy summary field `httpTransport:false` describes the earlier general
  RPC checks in this fixture; the final BLS-specific section unconditionally uses
  `HttpServer`, `PersistentRpc` and a TCP listener. It does not claim P2P transport.
- Native CLI, transition runner, daemon and dispatcher runner compile. Source and
  binary hashes were captured before observing final HTTP completion and rechecked
  afterward. Patch is relative to the recorded base commit and reverse-checked.

## Remaining gates

Catalog/export and automatic relay of pending BLS changes are not implemented.
The newly accepted message has not yet been sent through a real libp2p stream in
this tranche. Full multi-node delivery/inclusion, adverse network behavior,
queue reorg/pruning and admission write-fault coverage remain open. This is native
Nano framing, not Ethereum gossip compatibility or a public Beacon submission API.
EIP-4895/Capella and production remain partial; no deployment or real funds.

Build-log trailing whitespace was normalized for repository checks.
