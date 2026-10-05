# Resumed canonical archive audit

Parent checkout cf25c4dae7ac00ac0d2ba9a31e146f5db469f0d9, source-data producer 0948b747. Prior binary actual readonly audit FAILED `Capella slot work budget exceeded` with exact external checkpoint/finalized commitments; negative evidence retained.

New explicit local archive APIs finish bounded authenticated slot chunks, then validate full signed block/state transition and FFG checkpoint contexts. Audit retains canonical block, execution state, transaction/receipt index and external finalized-ancestor comparisons. Ordinary admission/replay budgets unchanged; no gossip admission, signing or persistence added. Historical fork-choice and standalone votes remain unverified.

ARM64 terminal 0/no OOM: fork_store and audit recompiled, other current-layout objects reused, main/test fresh. 146 checks (12 new oracle/budget/BLS/cancellation/tick checks across two variants), configured-runtime and scheduler PASSED. Existing image/volume reused.

Actual readonly capture against /private/tmp/nano-ready-target-cold-resume-20261005/data and independent stop commitments pending; fresh capture tool links same new core, source-file manifest recorded to verify immutability. AMD64 pending. Common WAN finality, wall-time/resource bounds and production readiness open.
