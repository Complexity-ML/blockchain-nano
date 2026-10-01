# Capella duty timing candidates — 2026-10-01

Compiled runtime source: `1e1e73208a96268e060c9ac92a871ec0eadab4f1`.
Mac native and Docker ARM64/amd64 each finished `--duty-timing`, `--scheduler`,
`--automatic-recovery` exit 0. Native compilation output is an excerpt (initial
build lines omitted); all qualification JSON results and terminal metadata are
retained. Docker logs are complete. Source hashes match the preceding ARM64
archive; its original failed fixtures remain immutable there.

New timing scope: admitted current-slot block or authenticated profile deadline
before the scheduler's attestation/sync phase; missing-block fallback, malformed
block rejection, no missed-slot backfill with legitimate next-slot duties,
duplicate polls and a real worker with full-slot configured interval.
Scheduler scope: 40 blocks, 272 votes, 1280 sync messages, finality epoch 3,
restart at slot 24 and 64 reopened signing journals. Automatic recovery scope:
16 blocks, 64 keys, retained independent signer witnesses/commitments.
These are in-process tests, not HTTP or WAN qualification.

Runtime `nano` was linked from the qualified core on each architecture; unchanged
P2P executables are reused. Each Docker image was assembled once from pinned
existing build/runtime images with no dependency download or source compilation
in the packaging step, then independently inspected and checksummed. Image ID,
architecture, revision label, user `10001:10001` and both hashes passed
`verifyPackagedImage`. Native runtime hashes are recorded in the manifest.

Next WAN case keeps 96/192-second partitions, strict monotonic 180-second complete
recovery read, two-second Beacon execution budget and disabled detailed network
trace. This code change warrants a new measurement; it is not established as
WAN M's cause or a WAN improvement. M and earlier failures remain preserved.
No production gate, consensus threshold or signing rule has been weakened.
Production, bridge, fair launch and public deployment remain unqualified.
