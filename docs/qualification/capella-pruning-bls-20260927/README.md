# Pending BLS change across Capella pruning — 2026-09-27

New scenario `consensus_capella_pruning_bls` uses two independent local execution
owners and 64 synthetic validators. Only the follower admits a signed withdrawal
credential change for validator 1; the proposer omits it during the 40-block loop.
Consensus states and signed votes must remain equal. Checkpoint tokens may differ
because the follower has an additional durable pending operation.

At finalized epoch 3, the test proves the operation's original admission root is
outside the retained pruning candidate. It prunes the follower through the actual
durable API, closes and reopens RocksDB, then checks:

- The pending signed bytes are unchanged.
- The serialized admission root now equals the retained finalized root.
- Resubmitting the same operation does not create another checkpoint.
- A proposal assembled by the restored follower includes the operation.
- The same signed block gives equal Beacon state, execution root and block hash
  on unpruned and pruned stores.
- Validator 1 acquires the expected execution withdrawal credentials.
- The applicable queue becomes empty while the unfinalized signed archive remains.

This exercises existing production code; no runtime fix was necessary for this
passing scenario. It does not prove subsequent payout, finalized archive retirement,
reorganization after pruning, slashing-proof preservation, native deposit pinning,
crash during pruning, automatic scheduling or production readiness. The inherited
primitive checks also cover cancellation, idempotence, history and network identity.

Result: passed in 34.06 s. Source/binary hashes captured before the test and verified afterward. Logs normalize trailing whitespace only.
