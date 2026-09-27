# Capella native pruning sync-failure recovery — 2026-09-27

The new macOS-only scenario first creates a native Capella chain with 40 signed
blocks and finality, then initializes four independent disposable databases with
the same execution history, consensus state and signed vote archive. Each has a
separate trusted recovery witness.

The child process injects EIO into the actual principal or witness WAL sync during
`prune_finalized`, verifies the reactor/owner/RPC fail closed, and exits without
running destructors. The parent either keeps the written suffix or truncates only
the newly written suffix of that disposable WAL. Expected old/new canonical
checkpoints are compared before witness recovery and afterward.

When the witness retained the candidate, recovery must restore pruning even if
the principal lost its suffix. If the witness lost the candidate before principal
publication, the old anchor must remain until an explicit retry. All four cases
must retain signed votes, execution tip/state and idempotent completion. Another
reactor reopening and an RPC query check the recovered state is usable.

This isolates the pruning commit itself. It does not inject between the clock
commit and automatically scheduled maintenance, test both stores losing earlier
committed writes, simulate physical power loss, or qualify Linux/VPS behavior.
The prepared database has authenticated signed votes but no pending BLS changes
or slashing proofs. No production readiness or storage-growth closure is claimed.

The initial build caught a test-only string/JSON comparison type error; the
comparison was made explicit before qualification. No runtime behavior was changed.

Result: all four dispositions passed in 67.06 s. Principal faults blocked eight checked operations; witness faults blocked six (raw principal storage remained readable). RPC failed closed in all cases. Source/binary hashes captured before testing and verified afterward; logs normalize trailing whitespace only.
