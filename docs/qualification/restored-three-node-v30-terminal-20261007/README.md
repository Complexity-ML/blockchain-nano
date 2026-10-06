# V30 terminal — 7 October 2026

Original controller session82206 exited1 at the prescribed900-second finality deadline. All three readiness checks had passed at22:01:56UTC. No restart, replacement binary, timeout extension or controller change occurred. All controller hashes still match. The run restored the prior inventories with explicit source budgets1k/1k/4k and target4k; the earlier preparation proof remains separate.

All three owners actually resumed duties: native16 keys/1 proposal/8 attestations; Docker Mac16/2/8; Docker VPS33/3/7. Stop receipts report healthy clocks and validators. Native exit0, both containers exited0/noOOM. This is positive bounded evidence for duty participation on the restored data, not successful network recovery.

The finalized checkpoint stayed at epoch1284/root0xccf88d18b2b98689ce47814d966ab42d10898eddd83a31bb2e3873ac5b2a2c48. Zero of the required three distinct advancing common checkpoints were observed. Different advancing heads do not satisfy this gate. Max validator poll was17392/24432/67874ms, compared with the configured short-slot workload; no claim of a sole cause follows from these counters.

The native stack sample and a separate regression identify a full cold decode in final durable canonical serialization despite the existing preflight memo. Its correction is qualified separately, never injected into V30. Further catch-up/transport/signed-operation costs may remain. Historical signed-vote contexts are not rewritten or erased to obtain convergence.

Public receipts, original events, states and artifact identities are retained here; raw local logs are preserved at the paths/hashes in summary.json. Private databases, keys, witnesses and full private requests remain on their host. New post-stop independent capture/pin audits are required before any further writer because this run signed and published new data. Neither old V28 pins nor pre-V30 captures can be silently reused.

All remaining production, EVM-profile, bridge/reserve/redemption, Doppler and independent-security-review requirements remain open. No launch, funds or public deployment.
