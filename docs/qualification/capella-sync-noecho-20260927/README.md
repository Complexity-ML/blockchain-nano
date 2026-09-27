# Capella relay echo suppression and known-root shortcut — 2026-09-27

## Change

Capella admission now remembers sender possession for accepted attestations and
sync messages in the exact per-peer, per-kind cache used by outgoing catalogs.
Previously the attestation wrapper populated a different Altair cache, and sync
messages had no sender-possession tracking, allowing accepted messages to be echoed
back. Only successful admission seeds knowledge. A late callback retains the old
session's cache object and cannot populate a replacement session.

The catalog relay consults that existing bounded knowledge before exporting the
payload. Known roots advance the validated catalog cursor without another core
export or send. Pagination, entry budgets, peer-session guards, fresh-message
validation and ACK requirements remain. Asynchronous advancement avoids recursion
through a large known page. Cache eviction still permits retransmission. Neither
inbound limits nor consensus timing/acceptance rules were increased or relaxed.

Completion telemetry records failureStage/reason, exports, skippedKnown, elapsedMs
and last decoded syncSlot. `delivered` is the existing count of traversed acknowledged
entries and includes skippedKnown; it must not be interpreted as new wire deliveries.
A missing ACK is an unknown outcome, not an explicit rejection or proven message loss.

## Qualification

Targets rebuilt: nano-p2p, catalog relay, block delivery, stream and core dispatcher
tests. Four CTests passed in 0.45 seconds. The catalog suite now contains 33 cases,
including mixed/all-known pages, no export for known roots, entry-limit preservation,
accepted/rejected/local-failure sender knowledge, and completion after session reset.
The tests use mocked ACKs; consensus evidence is separate below.

Two actual local libp2p peers with 32 disjoint validator keys each pass the existing
bounded distributed sync test. Same 210-second finality deadline, same admission
limits, no test injection of sync signatures, clean process termination. Common
finalized epoch 2 observed three times. **All eight sampled finalized blocks at
slots 9–16 include 32/32 committee positions**, including 15 or 17 remote positions
relative to their proposer. Signed blocks were observed identically on both peers;
BLS aggregates were independently verified against the fixture committee and domain.

| Observed measure | Instrumented baseline | Corrected run |
| --- | ---: | ---: |
| Positions in each of eight finalized samples | 21–30 / 32 | 32 / 32 |
| Block export calls | 5981 | 81 |
| Attestation export calls | 19863 | 237 |
| Sync export calls | 3736 | 1092 |
| Sync delivery-unconfirmed attempts | 315 | 119 |
| Sync explicit rejections | 43 | 10 |
| Sync export-unavailable attempts | 10 | 0 |
| Completed sync catalog attempts | 8 | 290 |

The runs have distinct genesis times and different attempt totals; raw counts are
observations, not a normalized benchmark or statistical guarantee. Full participation
is established only for these eight finalized samples, not every block or network.
Unconfirmed deliveries still require investigation. Budget pressure is plausible
but receiver-side reasons were not instrumented, so it is not declared proven.

## Remaining gates and evidence

Next: receiver-side failure attribution, sustained/asymmetric network conditions,
WAN and sync committee-period rotation. No production closure, public launch,
VPS deployment, additional funds or continuous flood.

`comparison.json` includes both runs' exact samples, counters and node timing.
The baseline has its own source/binary provenance in the adjacent diagnostics
folder. This folder's `source.patch` is the complete final delta from its recorded
base (including diagnostics), not an incremental patch atop the baseline patch.
Source/binary hashes were checked after final tests. Full logs retained; trailing
whitespace only normalized. Reproduction:
`build-release/nano-capella-transition-tests --sync-distributed-cli build-release/nano build-network/nano-p2p`.
