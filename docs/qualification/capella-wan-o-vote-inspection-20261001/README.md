# WAN O canonical inclusion inspection — 2026-10-01

Native database stopped after failed WAN O, inspected read-only with its exact
external checkpoint. Full canonical replay passes: 117 blocks including trusted
anchor, 332463 archived bytes, one transaction. Reused previously reviewed
inspector executable verified against its recorded SHA-256; no rebuild or new
network campaign. Parsed 116 signed archived blocks; byte/count totals match audit.

Epoch 14 contains 54 participation bit occurrences: 35 target the retained
canonical boundary, 19 another target. Epoch 15 contains 64 occurrences, 59 on
its canonical target. These counts are occurrences, not distinct validators or
stake sums. With 64 equal-weight fixture validators, quorum needs 43 distinct
participants; 35 occurrences cannot supply that many participants. Competing
targets cannot be added. This supports insufficient canonical target-14 inclusion;
it does not establish the sole cause of the split or recovery delay. The added
block-dependency transport did not restore that inclusion quorum in this run.

Votes outside blocks and historical fork choice are not replayed. No production,
full network or bridge qualification. Databases, node configuration containing
private paths, keys and journals stay outside Git; public inputs are identified
by hashes and preserved locally. Source inspector and public outputs archived.
