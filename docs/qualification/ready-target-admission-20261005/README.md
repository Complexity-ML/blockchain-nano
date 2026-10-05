# Ready target admission

Parent checkout 24f5417f172d44be81f6bdbdd02807e032ee1dae; unchanged-core negative test uses producer aa9c4a26. New test prewarms the included attestation target through gossip and the block parent through local duties, then requires immediate admission. It FAILED on prior code with `ready included target unnecessarily deferred local admission`.

Fix: after fully verified signed transition requests a missing target, an already completed cache frame is authenticated and reused; rebuild pointer view and finish admission in this call. Genuine unfinished slot work still returns false after bounded progress. Cache bound unchanged; no journal/profile/budget/layout change. Every contextual transition still checks the complete block.

ARM64 terminal 0/no OOM, 134 checks (four new immediate-admission/whole-Store oracle checks across two variants), configured-runtime and scheduler PASSED. Reactor only recompiled against fully qualified current-layout core; main/test freshly compiled and linked. Negative evidence preserved separately. Images/volumes reused.

AMD64 and actual failed-copy resume pending. Restart must keep advanced source journals and exact external-stop pins; automaticRecovery=false. Actual long-gap recovery, common WAN finality, resource bounds and production readiness remain unqualified.

## Actual failed-copy resume

Producer 0948b747 resumes a fresh copy of the FAILED cold activation's actual advanced data/journals, with exact external core/key stop pins and independent witnesses, automaticRecovery=false. It does not call the healthy-receipt helper with forged health: controller explicitly checks the known failed-validator stop, source container exited 0, source key inventory and monotonically advanced pins before runtime verifies them. Two-minute observation then controlled stop: terminal 0/no OOM, healthy validator, 9 proposals, 82 attestations, 348 sync messages, no failure. Core commitment advances; all 32 signer sequence floors verified against previous receipt; finalized epoch 75/root unchanged. Latest data/journals and stop receipt retained for next resume.

This proves scoped local recovery after the known refusal, not full canonical replay of the fourteen-hour gap, multi-node common-finality advancement, external publication, or production readiness. Prior failure evidence is preserved.

AMD64 producer 0948b747: terminal 0/no OOM, 134 checks plus configured-runtime and scheduler PASSED. Reactor alone recompiles against current complete new-layout core; main/test fresh. Nine source hashes equal ARM64/current checkout. Scheduler digest 0x1530f589ec8c4afa69ba9245c4585797067f73368eea316927e8494407442d04 / sequence 393 / finalized epoch 3 matches ARM64. Next: resumable canonical archive audit for the historical long gap, then restored common-finality advancement across native Mac, Mac Docker and VPS Docker with disjoint synthetic signer sets.
