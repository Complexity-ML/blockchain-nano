# Executable argument provenance

The historical ARM64 c10426b2 manifest contains 197 hashes, covering registered primary test commands plus nano-p2p. It does not record nano-altair-probe, nano or nano-finality-crash-fixture when supplied only as command arguments. Historical results remain scoped to those 197 before/after hashes; additional program before/after integrity is not retroactively proven.

The updated manifest records absolute executable arguments with ELF/Mach-O/PE/script headers. Required nano-altair-probe is always checked, even when missing/nonexecutable. An explicit NODE_EXECUTABLE option lets the full Docker runner pin nano even when it is not present in arguments. Runtime dependencies and programs embedded only in compiled test code are not automatically inferred.

Five new controls pass on macOS: helper inclusion, helper-only replacement, absent/nonexecutable required helper, and explicit node coverage. They also verify that a JSON data fixture with its executable permission bit set is excluded. The original Linux-only /usr/bin/test path failed two positive macOS controls; tool resolution is now portable.

Read-only Docker Linux/ARM64 inspection of the retained 417-case registry produces 200 hashes: all 197 historical entries are unchanged and three programs are added. An explicit-node invocation produces the same 200-entry manifest. No runtime tests, image build or source/binary modifications occurred. The initial expanded manifest had 202 entries because two data fixtures had executable bits; that observation is retained, and header validation corrects the classification. The initial host comparison expected only one additional helper and failed; the corrected comparison explicitly checks all three additional programs.

The active AMD64 c10426b2 campaign and queued 1ac57e25 corrective campaign keep their frozen source scripts. This change qualifies tooling, not the runtime or global production. It applies to subsequent full Docker qualifications; existing omitted helper hashes remain an explicit provenance limitation.
