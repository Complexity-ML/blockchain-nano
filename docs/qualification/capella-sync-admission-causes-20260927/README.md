# Capella sync admission causes — 2026-09-27

The reactor records fixed-size, saturating, mutex-protected counters for completed
sync message decisions. These are ephemeral diagnostics, reset on reactor restart,
and are never included in state roots, durable checkpoints, wire ACKs or fork
identity. Both local signing and network admission use this path; accepted counts
include duplicate accepted submissions, not unique messages. Exceptions/cancellation
are not reported as ordinary rejections. Direct aggregate admission is not counted.

Reasons report the first failed check: malformed wire width, slot overflow,
expired/future inclusion slot, selected head already at/after inclusion slot,
unknown validator, root mismatch, nonmembership, invalid BLS signature, or failure
to aggregate/merge. A rejection before the signature check does NOT prove that the
signature would have been valid. Aggregate failures are grouped, not fully attributed.

Counters are exposed in the existing authenticated nano_capellaStatus response
and the stopped-runtime receipt. No new endpoint or authentication change.
Existing bool decisions and exception behavior are retained.

The local sync qualification now checks exact counter increments for malformed
width, wrong root, overflow/future/expired slot, unknown validator, noncommittee
validator and corrupt BLS signature. A message targeting an already proposed slot
is rejected without changing the checkpoint. Stale checkpoint and cancelled calls
leave decision counters unchanged. The complete local scenario passes in 45.95s,
including existing assembly, BLS, cross-reactor export, worker and finality checks.

The distributed evidence below is bounded to two local peers, 32 disjoint keys
per peer, the initial committee period and the existing finality deadline. It
cannot establish WAN, committee rotation, scalability or production readiness.

Reproduce:
`ctest --test-dir build-release -R '^consensus_capella_local_sync$' --output-on-failure`

`build-release/nano-capella-transition-tests --sync-distributed-cli build-release/nano build-network/nano-p2p`

Source.patch is relative to the recorded base commit; provenance records source
and binary hashes captured before the distributed run and checked after both
runs. Full logs retained, only trailing whitespace normalized.


## Distributed result

Run exit 0: common finalized epoch 2, three stable observations, eight independently
verified finalized sync aggregates (slots 9–16), all 32/32 positions. Core counters:
node A accepted 972, root-mismatch 7, head-at-or-after-inclusion 1; node B accepted
978, root-mismatch 7, expired-slot 1. All other rejection counters zero. These 16
core rejections equal the 16 receiver sync rejected-ACK writes and sender sync
admission-rejected attempts in aggregate. No per-message correlation ID is claimed.
There were 957 accepted sync ACK writes; accepted core counters include local
signing and duplicates, so they are deliberately not equated to wire deliveries.

No budget rejection, local core error or failed ACK write was observed. Two sync
sender deliveries remain unconfirmed; that unknown-outcome limitation is retained.
No signature-check failure occurred, but messages rejected earlier did not undergo
signature validation. Root disagreement is measured; the exact network/head timing
cause of each disagreement has not been traced. These checks should not be relaxed
to make rejection counters zero. The previous run's 18 refusals cannot be classified
retroactively from these new counters.

Next substantive gate: committee-period rotation and asymmetric/partitioned network
qualification. Transport ACK ambiguity remains tracked. No public launch, funds,
VPS update, or production completion.
