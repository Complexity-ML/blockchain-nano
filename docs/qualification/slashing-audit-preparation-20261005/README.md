# Slashing audit preparation — 2026-10-05

The native read-only audit tool compiled and linked against the existing ab10b8ae runtime library. No node binary or Docker image was rebuilt. Three negative controls passed: an unslashed finalized member, a forged proof signature, and an incorrect external checkpoint digest are refused. This is preparation, not a qualified WAN sanction or production gate closure.

The first negative attempt failed earlier than intended because the tool compared the witness record token with the primary token. The witness has its own digest; its canonical recovery intent names the primary next token and snapshot. The corrected tool validates that intent and snapshot hash. The initial failure is retained alongside the corrected refusals. Existing databases were opened read-only; no recovery was authorized.

A positive finalized sanction, exactly one canonical inclusion/transition, replay after restart, common finality advancement and AMD64 execution remain unqualified. The current CI run 37258158600 was still in progress when checked; no terminal success is claimed.
