# Capella durable pruning — 2026-09-27

The local `prune_finalized` maintenance API publishes the qualified pruning
candidate through the normal execution-owner checkpoint path. Moving the storage
anchor now requires exact equality with the pure transformation of the existing
live store. Pending BLS admission roots are reconciled by the existing publication
logic. Before any database write, the complete durable envelope is restored and
validated, including archived signed votes, slashing proofs and pending changes.
An incompatible archive therefore refuses publication rather than discarding
its evidence. The network anchor remains fixed.

The extended primitive test cancels before publication, checks a single checkpoint
sequence increment, checks duplicate pruning does not write, closes and reopens
the owner/RocksDB database, compares all retained signed votes and network identity,
and executes the same next signed block as the unpruned reference. The execution
owner must expose the identical block hash and state root afterward.

This test uses the existing 40-block, 64-validator, two-owner local fixture. It
has signed votes but no pending BLS changes or slashing proofs at pruning time;
those archive paths still require dedicated pruning scenarios. Automatic scheduling,
native deposit-history pinning, crash/sync-error injection during pruning,
long-running bounded storage and actual disk-space reclamation remain open.
No public network deployment or production readiness is claimed.

Result: passed, 34.25 s. Source/binary hashes captured before the test and verified afterward. Retained logs normalize trailing whitespace only.
