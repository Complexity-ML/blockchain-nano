# Ready target admission

Parent checkout 24f5417f172d44be81f6bdbdd02807e032ee1dae; unchanged-core negative test uses producer aa9c4a26. New test prewarms the included attestation target through gossip and the block parent through local duties, then requires immediate admission. It FAILED on prior code with `ready included target unnecessarily deferred local admission`.

Fix: after fully verified signed transition requests a missing target, an already completed cache frame is authenticated and reused; rebuild pointer view and finish admission in this call. Genuine unfinished slot work still returns false after bounded progress. Cache bound unchanged; no journal/profile/budget/layout change. Every contextual transition still checks the complete block.

ARM64 terminal 0/no OOM, 134 checks (four new immediate-admission/whole-Store oracle checks across two variants), configured-runtime and scheduler PASSED. Reactor only recompiled against fully qualified current-layout core; main/test freshly compiled and linked. Negative evidence preserved separately. Images/volumes reused.

AMD64 and actual failed-copy resume pending. Restart must keep advanced source journals and exact external-stop pins; automaticRecovery=false. Actual long-gap recovery, common WAN finality, resource bounds and production readiness remain unqualified.
