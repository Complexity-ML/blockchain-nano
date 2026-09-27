# Authenticated execution-deposit prefixes — 2026-09-27

## Why native Capella pruning still needs work

`capella_native_deposit_vote_known` currently searches every execution ancestor.
Its voting-period selection also falls back to the storage anchor. Deleting old
records without retaining their deposit commitments can therefore change accepted
votes or the selected period vote. The previous native-pruning rejection remains
in force; this change does not enable it.

## Implemented prerequisite

`ExecutionDepositHistory::authenticated_prefix` reconstructs an earlier immutable
prefix from the retained append-only deposit tree. Fully retained subtrees are
shared; only the boundary path is rebuilt (at most 32 new nodes). The count is
bounded by the source history, the execution hash must be 32 bytes, and the
resulting count-mixed root must equal the caller's authenticated historical root.

The caller must authenticate the complete historical execution-hash/count/root
tuple from retained canonical ancestry. This function verifies the prefix contents;
it cannot establish execution ancestry from a hash supplied by its caller. An
empty execution block can legitimately share the same deposit count and root.

## Qualification scope

The existing test executes real deposit-contract transactions for a funded entry,
a top-up and an invalid-PoP entry, plus a conflicting branch and an empty block.
New assertions reconstruct all four prefixes (counts 0–3), compare snapshots and
six historical Merkle proofs exactly, and bound callback visits to one 32-level
tree boundary. They reject a foreign branch root, oversized count, short hash and
tampered commitment. Cancellation in the middle of the path must leave the source
unchanged. The empty-block case checks recovery of the earlier execution binding.

## Next integration

Persist authenticated ancestor deposit commitments and slot/ancestry metadata,
use prefixes for old vote lookup and mandatory proofs, and retain voting-period
selection across storage-anchor movement. Native Capella pruning, long-running
storage bounds and crash qualification are still incomplete. No EIP closure or
production-readiness claim follows from this primitive.

Result: passed in 0.39 s. Source/binary hashes were captured before the test and verified afterward. Logs normalize trailing whitespace only.
