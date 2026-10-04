# Runtime candidate pair and scoped core CI — 2026-10-05

ARM64 image: 272,168,394 bytes. AMD64 image: 251,143,515 bytes, actual build/controller exit 0. Both use UID/GID 10001 and preserve the qualified Nano/P2P binary hashes. Eleven license hashes, library resolution and read-only/network-disabled help execution passed. Immutable identities are in manifest.json. No public or private consensus network was started for this packaging step.

Runtime sources are equivalent to 59fa6530 and main 4eb31d3a. Reused entry/core/storage/P2P object provenance remains explicitly documented in the ARM64 companion proof; no complete fresh build at main is claimed. Apt versions are recorded, Ubuntu index pinned; mutable apt repositories and a hermetic dependency rebuild remain open.

GitHub run 37242291266 core completed successfully: exactly nine registered tests, no failures or skips, 14.35 seconds CTest time. Actual XML, selection, logs, compiler/package versions and executable hashes are retained under ci-core/. This is a scoped CI result, not the 427-test campaign or a new full pruning qualification. Operations succeeded; network was still in progress at capture.

Next gap found by source inspection: the multi-host harness currently refuses packaged backup restoration and hardcodes /build/nano for offline audits. Its backup helper also runs as root and creates mode-700 restored data; packaged nodes use UID 10001. Candidate-image restore support therefore needs explicit audit command/user selection and ownership of only the new restored copy, before running the Nano2 WAN restoration. Existing source/archives and independent witnesses must remain untouched. This gap is not hidden by packaging success.
