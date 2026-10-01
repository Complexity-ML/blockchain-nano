# WAN O individual signers identified — 2026-10-01

Read-only native stopped database, exact external checkpoint, previously archived
canonical replay verified. Diagnostic C++ inspector reuses the already compiled
core and verifies each single-participant BLS signature against the retained
fixed fixture registry, with target-epoch fork domain and validators genesis root.
All included individual votes for target epochs 12–15 match exactly one key.
Archived fields/counts/roots match the previous inspector. Canonical replay
separately checks committee position, eligibility and transitions; identity alone
is not an independent authorization proof. No aggregate attribution implemented,
no historical off-chain votes or fork choice replay.

Epoch 14 canonical target has 35 DISTINCT identified keys: Mac native 22,
Docker Mac 11, VPS 2. The earlier target has 19 distinct keys: VPS 18, Docker Mac 1.
Only 54 of 64 fixture keys appear in included epoch-14 votes; the other ten are
not inferred absent from all network traffic or journals. No matched key occurs
on both epoch-14 targets in this canonical archive. Quorum requires 43 equal-weight
keys for one target. Epoch 15 canonical target has 59 distinct keys (22/18/19).

This shows most INCLUDED VPS epoch-14 signatures were committed to an earlier
boundary. It does not prove why its local fork choice converged late, when each
block/vote was delivered, or why ten keys have no included vote. Next diagnose
head/branch delivery ordering and authenticated fork-choice votes before another
WAN campaign. No production or global network claim, no signing policy change.

First diagnostic compile used object paths relative to the repository rather than
the build directory; its missing-file error is preserved. Correct paths compile
successfully. No runtime executable/image was rebuilt or live campaign restarted.
Only public outputs/source/fixture ownership are archived; private files stay out.
