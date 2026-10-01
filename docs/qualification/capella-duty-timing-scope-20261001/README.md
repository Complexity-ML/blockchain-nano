# Correction of timing qualification scope — 2026-10-01

Baseline `030871aad8604ffaa38ae6dbb67f4e8a21faac98` already had a
`sync_current` guard that rejects skipped-slot signing before
`ceil(slot_duration_ms / 3)`. The retained baseline source proves this.
Attestation duty preparation also already enforced its profile deadline.
The WAN N profile is 6000 ms/slot and 2000 ms attestation due: the old guards
already prevent early old-head signatures in this profile.

Therefore the preceding archives/roadmap wording claiming that sync signatures
could happen immediately is incorrect. Preserve those archives as historical
records, but interpret their passing no-early-signature tests as qualification
of existing guards together with the new scheduler filtering. This was not a
newly closed signature vulnerability or an established cause of WAN M.

Runtime `1e1e732` moves the scheduler's readiness check before duty enumeration
and sync attempts. It avoids premature context work and observes the existing
profile deadline; it does not replace the final signing guard. Its `start()`
change bounds a full-slot configured poll interval, and that real-worker behavior
was separately qualified. A WAN improvement remains unproven.

WAN N is running with frozen runtime, harness, images and configuration. This
correction changes documentation only. Do not rebuild or rerun the same candidate
to obtain green; preserve N's actual result. Production/EIP criteria stay open.
