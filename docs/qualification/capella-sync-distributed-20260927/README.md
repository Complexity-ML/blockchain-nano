# Capella sync over real libp2p — 2026-09-27

## Result

Two actual local nodes, each owning 32 distinct fixture validator keys, exchange
sync messages through the production daemon/private RPC path. No sync message is
injected by the harness. The strengthened `--sync-distributed-cli` scenario passed
with three observations of common finalized epoch 2 within the original 210-second
deadline. Both processes stopped cleanly and reported healthy validator loops.

Eight finalized blocks (slots 9–16) contain 6–10 positions signed by keys owned by
the other node relative to the proposer. Each exact signed block was observed on
both peers before pruning, then reached via ancestry from the common finalized
root. The harness independently verifies the aggregate BLS signature with fixture
committee keys, signed parent root and sync domain. It limits this analysis to the
initial committee period, and requires at least three qualifying blocks.

Common finalized root:
`0x2426ece3a52625908ac5f643e2be7c33b6d2a7b46c450695b1545c37213baf52`.

## Limit exposed and correction

The initial run reached common finality 2 but failed a Beacon HTTP read; it remains
an **unsuccessful qualification run**. Its generic error did not retain the exact
endpoint/status. Source inspection found that `signed_block_locked` and archive
verification unconditionally excluded the current anchor. After automatic pruning,
a previously admitted signed block could consequently become unavailable via the
finalized Beacon header/block APIs despite its signature remaining on disk.

Both exclusions are removed. A record must still exist, match the retained header
and body root, and pass proposer signature verification against authenticated
state. An initial anchor with no signed record remains unavailable; no signature
is fabricated. Archive/storage retention and fork-choice rules are unchanged.
The automatic-pruning scenario now asserts signed header/full-block availability
for every newly moved anchor on both reactors and passes in 67.76 seconds. The
subsequent distributed confirmation passes. The test's HTTP errors now include the
endpoint and status for future failures. An earlier compile failure was a missing
BLS declaration include in the new harness and is retained separately.

## Performance and unresolved delivery

The eight sampled aggregates contain **21–27 of 32 positions**, not full committee
participation. The confirmation logged 365 incomplete sync catalog attempts and
11 completed ones. These are attempt counts, not message-loss rates: attempts can
partially deliver, encounter expiry, or acknowledge already known roots. Exact
causes of each incomplete attempt are not yet established.

Local signing counters are 480 and 510 messages. Maximum validator poll durations
are 755 ms and 605 ms; no poll exceeded a slot. Three slot-boundary exits total
still occurred. This is one local run, not a general latency guarantee.

Next priority: instrument and reduce incomplete sync delivery and measure inclusion
coverage without weakening deadlines or starving attestations. WAN, sustained load,
committee rotation, adverse network conditions and complete production readiness
remain open. No VPS deployment, public launch or real funds.

## Reproduction and artifacts

Build `nano` and `nano-capella-transition-tests` in `build-release`, and `nano-p2p`
in `build-network`; run the command in `provenance.json`. The pruning regression is
`ctest --test-dir build-release -R '^consensus_capella_pruning_automatic$' --output-on-failure`.

`initial-provenance.json` and `initial-source.patch` bind the failed run's binaries
and harness. `provenance.json` and `source.patch` bind the corrected confirmation;
hashes were verified after both final tests. `analysis.json` separates initial and
confirmation counters. Complete logs are retained, with trailing whitespace only
normalized. Historical success does not override the incomplete participation
reported here; no EIP or production gate is declared fully closed.
