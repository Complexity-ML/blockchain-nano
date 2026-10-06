# Capella immutable record leases and cold graph transitions — 2026-10-06

Base: 2355c33d1748b36b7d2c8e3b80eae66c7b860faf. No node or image deployment.

CapellaForkStore now owns record handles. Head selection returns a shared immutable lease; metadata remains separate from heavy records. Candidate mutation detaches shared records. Cold records authenticate retained Beacon and execution metadata against the loaded row. All current runtime initializers still produce hot records.

Docker ARM64 original 73055 and AMD64 3478 terminated 0. Twenty-four affected core translation units rebuilt using compiler dependencies, copied parent archives, fresh transition fixture and link. These are incremental core builds, not clean release builds. Existing images and volumes reused, network disabled. Six modes pass on each architecture: cold-graph, owner-witness, configured-runtime, automatic-recovery, pruning-primitive, slashing-reorg. Additional ownership originals 93815/41260 terminated 0. Program/parent integrity checks pass. Public AMD64 collection 60347 terminated 0; public hashes collected separately from VPS.

The cold fixture replaces all graph entries with authenticated persisted references and compares fork head, canonical snapshot, clock transition, signed child transition and copy-on-write preservation to a warm oracle. Existing cache and durable migration scenarios also pass. It does not exercise runtime cold activation, graphs exceeding 1024 entries or WAN finality.

Final worktree matches frozen source-v9. Core/transition qualification used source-v6; the five v6-to-v9 differences are audit tools only (see source-final-verified.json). ARM tool syntax was repaired: first 50834 and 51065 failed; 61836 terminated 0 for the three corrected tools, remaining unchanged tools passed in v8. Tools were not executed or compiled on AMD64 in this lot. Initial core/fixture/driver failures 63947, 67995, 82283, 23787 and partial v3 source preparation remain historical failures; corrected fixture 11520 terminated 0. No assertion weakened to hide a runtime failure.

Open: runtime cold activation, owner serialization/concurrent readers of the cache, compact writer/restorer avoiding full flat expansion, graph capacity 1024, encoded versus physical memory bounds, transient decode resources, cold fragment garbage collection, full clean release and tool qualification, native node integration, common WAN finality after V28 FAILED. No EIP-wide, bridge, Doppler, production or independent security qualification is added.
