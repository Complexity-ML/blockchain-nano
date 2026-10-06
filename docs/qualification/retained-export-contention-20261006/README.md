# Retained block export during admission — 2026-10-06

Parent core 05649987; worktree based on d0044bf6. Only the reactor object is replaced in the qualified parent core archive. Exact source/program hashes and saved compile/link commands are retained. No image rebuilt, no owner database exported, no signing journal reset.

## Result

Native Mac and Docker ARM64/Mac + AMD64/VPS reproduce the old blocking path (baseline exit 1, expected admission-lock diagnostic). The candidate `--beacon-reads` passes (exit 0): retained signed export completes while admission holds its mutex; an exporter paused after capturing publication permits a concurrent commit and returns exactly the prior authenticated snapshot/token. Docker `--wire` passes on both architectures, including byte/checkpoint bounds, missing unsigned anchor, and export/admission checks.

Docker `--historical-export` initially exits 1 because the frozen fixture subset omitted the Ethereum deposit contract prestate JSON. That failure is preserved. The exact fixture is added with its digest; only this scenario is rerun with unchanged program/core (supplement exit 0 on both architectures). It verifies 22 historical exports, finalized ancestry, bounded proof resume, signature rechecking, cancellation, pruning and reopen, with five negative cases. The original controllers remain exit 1; no claim of an originally successful full run.

## Implementation and limits

Retained block export captures one durably published immutable store/token, authenticates disk archive/header/body/proposer signature using the existing Beacon verifier, and derives the envelope identity from that same store. Historical exports and mutable inventories retain the lock/proof path. Storage I/O still has its own lock. No consensus rule, capacity or trust check is relaxed.

Runtime `nano` and paused-capture helper linked with the new archive on all three environments; `nano --help` is exit 0. This is runtime linkage, not network qualification. Relay remains 76bb2468. v9 remains FAILED with no new common finality. This evidence does not establish the WAN failure cause, full-store liveness, successful three-owner restoration/finality, physical DB portability or production readiness. Next live campaign must use exact post-v9 owner commitments, preserved separately.
