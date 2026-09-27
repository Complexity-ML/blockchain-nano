# Capella bounded dependency collection — 2026-09-27

Base revision `635b8f1` plus accompanying changes.

collect_capella_sync_blocks follows SSZ-authenticated parent roots from requested
tips to trusted local roots using an injected fetch callback. It is iterative,
deduplicates common ancestors, bounds blocks and aggregate payload capacity,
checks cancellation before/after fetch, then returns parent-ordered payloads.
Missing data or exhausted budgets fail preparation before any consensus admission.
The callback receives a byte allowance and cancellation function; a real transport
must honor those before allocating/reading. Oversized returned capacity is rejected.

The enriched wire test passed in 5.73 s. Starting from two sibling tips, collection
retrieves all nine blocks with exactly nine successful-source requests. These
collected bytes are passed through follower admission, followed by votes and
slashing, and the sixteen-message convergence checks remain green. New failures
cover unavailable source, two-block budget and cancellation before I/O.

Fetching is callback-driven in-process here, not libp2p. Signatures, state and
proposer validation remain mandatory at admission. Peer selection, wire framing,
retry scheduling, stale catalog/pruning recovery and distributed finality catch-up
remain open. No public activation or real funds.
