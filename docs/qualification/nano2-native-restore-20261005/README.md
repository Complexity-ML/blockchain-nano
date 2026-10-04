# Native Nano2 storage and local restoration — 2026-10-05

Only storage.cpp compiled from runtime source equivalent to 59fa6530. A new static library and Nano/test executables were linked using preserved qualified native objects and libraries from 092a153c. Object-by-object comparison confirms only storage.cpp.o changed. Original executables/library/object hashes remained identical. No original build directory mutated.

Actual native Capella pruned physical-copy restore exited 0: 64 signer pins/journals resumed with retained independent witnesses, conflicting proposal refused before signing, local finality advanced 2→7; source backup unchanged. This is local transport=false, power-loss=false. Native P2P is reused unchanged. No full fresh native dependency build claimed.

The packaged harness final source rehash now selects its shell entrypoint/user explicitly, as the archive helper does. Actual AMD64 source comparison passed against the previous independent byte manifest, with source/proof mounted read-only. No restore/audit rerun or image rebuild. These prerequisites do not prove the subsequent three-node WAN recovery; that campaign remains to run.
