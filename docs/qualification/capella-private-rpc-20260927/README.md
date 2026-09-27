# Capella private RPC dispatch — 2026-09-27

Base revision `a882659` plus accompanying changes.

Rpc accepts an explicitly injected shared CapellaRuntime, mutually exclusive
with Altair/Bellatrix services. Capella sets requires_private_bind/local_only,
therefore the existing HTTP constructor requires loopback. Every method requires
the configured nonempty private bearer token. Token holders are one administrative
principal, not distinct authenticated P2P peers. RPC lock is released before calling
consensus, catalog and admission services. Clock health gates requests.

Methods: nano_capellaStatus, Submit, Job, Cancel, Forget, Jobs, RotateSession,
Export, CatalogOpen, CatalogPage and CatalogClose (each with nano_capella prefix).
The first parameter is always the private token. Submit takes session, positive
canonical sequence and envelope. Job/Cancel/Forget take session and sequence.
Export takes kind and canonical root. CatalogOpen takes kind; Page takes handle,
nullable root cursor and positive count; Close takes handle. Pages are capped at
1 MiB JSON. Export raw SSZ is limited to (16 MiB - 4096)/2 for hex framing.
Status explicitly reports that the network protocol is not activated.

Configured-runtime scenario passes in 21.43 s: wrong-token status/jobs/submit/open
refused, identity checked, submit/job yields accepted duplicate without checkpoint
mutation, export matches exact envelope, catalog open/page/close works, closed
handle and malformed root rejected. Existing receipt/lifecycle checks pass.
General RPC regression passes in 6.27 s after rebuilding its binary.

This qualifies Rpc::handle dispatch, not HTTP sockets or distributed transport.
No CLI runtime activation added. Next: HTTP admission/bind qualification, CLI
configuration, peer synchronization/dependency ordering, mixed-fork migration.
No public activation, real funds or production-readiness claim.
