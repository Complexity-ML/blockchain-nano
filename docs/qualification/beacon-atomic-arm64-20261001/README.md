# Atomic Beacon publication — ARM64, 2026-10-01

A coherent immutable store/token is preallocated before durable commit and atomically installed after success. Beacon reads capture it without the admission mutex. Admission, signing and persistence remain serialized. Invalidating/failing the reactor clears the readable publication; the latched failure diagnosis remains available. Restart and witness recovery recreate the admitted publication after verification.

Signed archive reads use the captured immutable store and the DB mutex. The digest, encoding, slot, SSZ, roots and signatures still authenticate through the original verification function. Missing/pruned data never falls back to a new head. DB-lock contention and projection cost can still occur; no lock-free-hardware or zero-latency claim.

Docker ARM64 exit 0: NEW actual admission paused under mutex, old-publication and archive reads complete, cancelled writer publishes nothing. Prior reader/admission overlap and HTTP diagnostics pass. Four clock fail-closed cases and configured automatic witness recovery pass (16 blocks, 64 keys). No amd64/WAN/production qualification yet; L and all prior failures preserved.
