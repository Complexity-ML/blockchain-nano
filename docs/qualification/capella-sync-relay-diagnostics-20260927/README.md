# Relay diagnostics baseline — 2026-09-27

Instrumentation-only run from the recorded base commit, before echo/known-export
corrections. Catalog completions retain the original failure stage, a reason,
export-call count, elapsed milliseconds and the last decoded sync signed slot.
Unconfirmed delivery is deliberately distinct from explicit admission rejection;
it does not establish packet loss, timeout or budget denial as the cause.

The catalog relay unit test passed in 0.36 s. The unchanged two-peer
`--sync-distributed-cli` scenario passed with common finalized epoch 2 and eight
finalized blocks with remote sync signatures. Participation was 21–30/32.
Sync attempts: 315 delivery-unconfirmed, 43 admission-rejected, 10
export-unavailable, 8 successful. Export calls: 5981 blocks, 19863 attestations,
3736 sync messages. These are calls/attempts, not distinct messages or loss rates.

`source.patch` reconstructs this instrumentation-only source from the base recorded
in `provenance.json`. Source and binary hashes were verified after the run before
further edits. Complete logs and `diagnostic-summary.json` retain the baseline.
The subsequent correction and its limitations are in
[the no-echo qualification](../capella-sync-noecho-20260927/README.md).
