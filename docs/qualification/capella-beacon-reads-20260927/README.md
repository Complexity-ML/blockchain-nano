# Capella Beacon read projection — 2026-09-27

Base revision `20cdb90` plus accompanying source changes.

## Implementation

Read handlers for version, genesis, pinned profile spec, head state root/fork and
head-state finality checkpoints; signed headers and full signed blocks selected by
head, finalized, root or canonical slot. Full blocks include JSON and exact SSZ,
version `capella`, execution withdrawals and BLS-to-execution changes. As in
Bellatrix, finality-checkpoint responses report the actual head state rather than
substituting fork-choice pulled-up checkpoints. Unsupported routes return 404.

The coordinator holds its publication mutex throughout Store selection, archive
retrieval/authentication and response construction. Signed bytes must match the
stored header/body/root/slot and proposer BLS signature. Missing signed anchors
are explicit 404s, not fabricated headers. An invalidated coordinator cannot serve
Beacon reads. The configured runtime exposes the handler with a clock-health gate.
Spec export identifies the native profile by digest; it does not invent Ethereum
fork epochs or advertise an unconfigured Ethereum preset.

## New coverage

- Capella branch scenario checks JSON/SSZ equality, execution hash/state/transactions,
  decimal withdrawal fields, head/root/slot selectors, state finality, profile
  withdrawal limits, 400/404 behavior and read cancellation without publication.
  Six committee votes change branch twice; both siblings' canonical/finalized flags
  are checked. BLS-change JSON fields are checked on the authenticated initial
  signed block, which is an unsigned-archive anchor in this scenario; later served
  blocks must not repeat that change.
- Configured runtime handler reads match the execution owner over 16 blocks and
  witnessed restart (existing scenario extended with new assertions).
- Scheduled 40-block finality checks the actual finalized signed header, and Beacon
  reads close after a backwards clock invalidates the owner.
- Bellatrix read/header regressions exercise the shared declarations and renderer
  overload integration.

## Initial failures retained

The first run had two failures. The Capella test mistakenly expected the anchor's
BLS change in the subsequent withdrawal block; the assertion was corrected to
check each event in its actual block. No renderer correction was needed. The
existing Bellatrix HTTP test could not bind its loopback socket inside the sandbox;
its isolated rerun with local socket permission passed. Initial logs are retained.

## Limits

Capella reads are exercised through C++ handlers, not a Capella HTTP server or P2P.
The existing Bellatrix regression is the only HTTP binding exercised here. Node
syncing and peer status remain unsupported until actual peer observations exist.
CLI dispatch, private RPC admission worker, P2P and live mixed-fork Store migration
remain open. This is a partial Beacon API, not full Beacon compatibility or public
production activation.

## Results

Final Capella gates: two suites passed in 31.26 seconds, with 14 read-check
batches over nine blocks and six votes/two reorgs. Earlier in this tranche,
configured runtime passed in 21.46 seconds and Bellatrix headers in 0.42 seconds;
Bellatrix HTTP rerun passed in 1.22 seconds. Five distinct suites are thus green,
with the two initial failures and their resolutions retained above.
