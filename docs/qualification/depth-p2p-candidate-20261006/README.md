# Depth fallback P2P candidate: native / ARM64 / AMD64

P2P producer 81e5ab31, core remains b5ee00ac. Fresh P2P compilation/linking
completed on native Mac, Docker ARM64 and Docker AMD64 using existing dependency
flags/volumes/images. Separate output folders preserve previous binaries. Parent
Docker binary hashes and qualified source hashes rechecked unchanged.

ARM64 reran the two new catch-up/rejection scenarios; its earlier 90 scenarios
are reused only because the three tested header hashes match exactly. AMD64
ran all 17 + 45 + 28 existing scenarios and both new scenarios successfully.
No Ethereum/consensus validity follows from the modeled ACKs.

Both initial Docker controllers exited 1 AFTER successful compilation/linking
and tests because they incorrectly expected `nano-p2p --help` to exit 0. This
binary has no help option: missing required arguments print Usage and exit 1.
The original failure records are retained. A separate tail verifier confirms
compile/link/test exits, parent/source hash preservation, and the expected
negative CLI exit 1; verifier exits 0. No object or test suite was rebuilt to
repair that assumption. Native logs likewise preserve compile 0/link 0/CLI 1.
This is a negative CLI check, not live networking qualification.

Compilation starts only after real v6 node stops; v6 remains a failed full-window
run on old P2P 8adbabff. No automatic deployment/restart or image rebuild.
The registered new CTest target was manually compiled with existing head-test
flags/link dependencies; fresh CMake configuration/CI is not established here.
Next: use post-v6 audited commitments with these P2P hashes in a real restored
three-node finality campaign. Production and all broader criteria remain open.
