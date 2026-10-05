# Authenticated bounded empty-slot continuation — 2026-10-05

Parent worktree: e6354658; parent core: 6b7a437e. New source changes are recorded by the enclosing commit. ARM64 Docker reuses the existing image and build volume, network disabled, one CPU, 2 GiB memory. No image build, source database mutation, signing or journal writes.

`CapellaSlotPreparation` authenticates the original SSZ post-state against its real block header. It keeps one private derived state and advances at most the requested number of slots per call. An unfinished chunk is discarded on cancellation; previously completed chunks remain reusable. The original block header/post-state remain unchanged. Advanced SSZ cannot be admitted as that original block's signed state. Caller serialization and head rebinding are required; this helper is not a persistent cache or a peer admission API.

Docker ARM64 terminal exit 0, no OOM; 11 scenarios PASS. Three chunk budgets (1, 3, epoch+1) produce exact byte/epoch-marker equality with monolithic processing across two history windows. Completion is idempotent. Mid-chunk cancellation preserves the prefix and subsequent resume matches the oracle. Backwards target, zero work/trial bounds, wrong parent state-root, and advanced-state impersonation are refused. Admitted original parent is unchanged and still authenticates.

Scope: new primitive only. Runtime duties, proposer/block construction, incoming block/vote admission and target contexts are not integrated yet. There is no successful cold duty recovery, WAN finality, AMD64 qualification or production readiness claim. The previous real cold recovery failure remains open.

The incremental archive replaces only consensus_capella_fork_state.cpp.o in a copy of the 6b7a437e core. The new class is not added to any existing object's layout. The transition-test translation unit is freshly compiled. Parent binary/archive/test hashes are checked unchanged.
