# Sync precollection timing — 2026-09-27

## Defect and correction

The prior sync collector accepted a message only during its inclusion slot,
requiring validators to wait until proposal time to disseminate signatures.
Altair expects signing and broadcast during the preceding signed slot.
Source: [consensus-specs v1.5.0, Altair honest validator](https://github.com/ethereum/consensus-specs/blob/v1.5.0/specs/altair/validator.md#prepare-sync-committee-message).

Admission now accepts a message during its signed slot and the following inclusion
slot, while retaining the existing BLS/domain, advanced-state committee and selected
branch validation. Aggregate collection accepts the corresponding proposal window.
Two separately keyed proposal-slot contexts preserve current and next aggregates;
precollection cannot replace the current proposal's participation. Message export
uses the same two-slot window and selected parent. Successful publication discards
older contexts; at most two committee-sized message maps and two aggregates remain.
All map allocations happen before publication and swaps are non-throwing. The
unchanged-head historical root handles skipped slots. Cache remains non-durable.

This supersedes the single-context retention described in the previous export
qualification; it does not claim Ethereum gossip subnet compatibility.

## Verification

Release build of `nano-capella-transition-tests` passed. CTest
`consensus_capella_local_sync` passed in 44.95 seconds:
40 blocks, 272 votes, finality epoch 3, two RocksDB owners, two database reopens,
64 signing journals reopened.

New test paths construct independent BLS sums, submit and export individual
messages before ticking to the proposal slot, deliver those bytes to the second
reactor, then check exact inclusion bytes after the slot boundary. Another case
retains both current and next message contexts for a skipped-head situation and
checks that the current aggregate survives. Existing malformed messages, wrong
signatures, overlap handling, cancellation, catalog budgets and invalidation after
head change remain covered. Future-window negative controls now use a slot beyond
the newly permitted precollection window. No validator signing key is used by the
production scheduler yet; test keys generate the messages.

## Open gates

Production signing/scheduling, one-third-slot timing, committee-period rotation
with precollection, real libp2p delivery before proposal deadline, WAN, asymmetric
partitions and performance remain open. The current test advances synthetic clocks
and relays via reactor APIs, not network sockets. No EIP closure, production launch,
VPS update or real funds. Prior network binaries/tests were not rebuilt or rerun:
this tranche changes the core's cache and timing, not network source.

`provenance.json` binds the tested sources and binary to a base commit; all hashes
were checked after execution. `source.patch` retains the exact tested delta.
Logs have trailing whitespace normalized only.
