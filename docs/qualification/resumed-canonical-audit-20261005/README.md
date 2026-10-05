# Resumed canonical archive audit

Parent checkout cf25c4dae7ac00ac0d2ba9a31e146f5db469f0d9, source-data producer 0948b747. Prior binary actual readonly audit FAILED `Capella slot work budget exceeded` with exact external checkpoint/finalized commitments; negative evidence retained.

New explicit local archive APIs finish bounded authenticated slot chunks, then validate full signed block/state transition and FFG checkpoint contexts. Audit retains canonical block, execution state, transaction/receipt index and external finalized-ancestor comparisons. Ordinary admission/replay budgets unchanged; no gossip admission, signing or persistence added. Historical fork-choice and standalone votes remain unverified.

ARM64 terminal 0/no OOM: fork_store and audit recompiled, other current-layout objects reused, main/test fresh. 146 checks (12 new oracle/budget/BLS/cancellation/tick checks across two variants), configured-runtime and scheduler PASSED. Existing image/volume reused.

Actual readonly capture against /private/tmp/nano-ready-target-cold-resume-20261005/data and independent stop commitments pending; fresh capture tool links same new core, source-file manifest recorded to verify immutability. AMD64 pending. Common WAN finality, wall-time/resource bounds and production readiness open.

## Actual stopped-data capture (ARM64 PASSED)

Fresh capture tool linked against producer 2795d4b5. Readonly container terminal 0/no OOM: 378 canonical blocks, two transactions, every execution state/index/receipt, exact checkpoint and external finalized ancestor verified. Independent primary witness and 32 signing journals/witnesses verified with external sequence floors; no retired signer. 873 source file sizes/digests unchanged. Historical fork-choice/standalone votes still unverified, no network finality claim.

## Physical copy pending approval

Automatic approval review rejected initial whole-data export to VPS due potentially sensitive internal state/signing metadata and insufficient dedicated transfer authorization. No transfer executed. Initial local allowlist checks detected `data/peer.key`; reviewed archive excludes that private P2P key and all env/private-key files. Prepared gzip: 14,568,786 bytes, 876 files, SHA256 0b34a3536de8f771c8577f5e9257d1aa6e75c28ecbf132119a843cf557fe6b54. Manifest retained, actual database archive stays local/private. Dedicated user approval requested for isolated readonly AMD64 audit; no signer will be started. This scope tests database/journal portability, not transfer of private P2P identity or full network qualification.

AMD64 producer 2795d4b5: terminal 0/no OOM; fork_store/audit recompiled against current-layout core, main/test fresh; 146 checks plus configured-runtime and scheduler PASSED. Ten source hashes equal ARM64 and checkout. Scheduler digest 0x1530f589ec8c4afa69ba9245c4585797067f73368eea316927e8494407442d04 / sequence 393 / finalized epoch 3 matches ARM64. Physical copy audit on AMD64 awaits dedicated user transfer approval; source-code regression is complete and is not that portability qualification.
