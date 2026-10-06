# Canonical durable-wrapper memo — 7 October 2026

Parent checkout: d7009a1f9dcd3662abb781ae1f4a278341d6ab72. Core parent archive: catalog-preflight-*-20261006 (coherent e33ad89e plus qualified runtime catalogue guard). Only consensus_capella_reactor.cpp is recompiled into a copied archive; fixture is rebuilt, programs relinked. Existing images/volumes and parent binaries preserved. No image rebuild or live actor replacement.

The final canonical reserialization in restore_durable did not forward the authenticated reference memo. A one-second native V30 stack sample found the validator poll in this path; this identifies a concrete remaining full cold decode, not proof that it is the only cause of WAN finality failure.

The fix forwards the local memo only for cold snapshots. Startup has no memo and still fully decodes. Every cold blob is still fetched/hashed by the preflight; graph rules and the exact canonical envelope equality are still checked. Signed votes, slashings, BLS changes and exits retain their decoders, contextual/signature checks. The publication-local memo remains installed only after successful durable commit. No consensus rule, fee, time or signer pin changes.

New regression: runtime with 32 blocks, restart, finalized pruning and signed-vote wrapper, followed by a one-millisecond tick without changing graph membership. It requires exactly three certificate-reuse passes for every retained record and zero new heavy cold validations, as well as one durable sequence increment. This fails at the named assertion on the old library and passes on the corrected library. The positive run continues through catalogues, admission checks and clean runtime stop. The same compiled fixture is linked against both libraries on each platform.

Timings are single scoped runs, not a performance benchmark. The test uses 18 retained records/80 signed votes, not a historical 1024+ graph or a WAN qualification. CTest gate is added in project source; direct invocations below provide execution evidence, not a fresh full-project CTest result.

Writer/preflight adversarial and cold fault gates retain mutated/missing blob, metadata/profile mismatch, cancellation, unchanged failed memo, primary/witness restart, four EIO and three SIGKILL boundaries. They do not demonstrate physical power loss. Historical failures and all production/Base-Nano/independent-review gates remain open. The live V30 campaign uses the old fixed binaries throughout; its result must be reported separately.
