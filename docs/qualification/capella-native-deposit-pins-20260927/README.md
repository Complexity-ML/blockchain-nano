# Native Capella deposit ancestry pins — 2026-09-27

## Implementation

Native pruning now retains canonical ancestor Beacon/execution headers and deposit
count/root commitments, newest first, back to the original network anchor. Full
historical EVM/Beacon states are not stored in these entries. Validation checks
both header chains, original anchor continuity and every deposit prefix against
the retained anchor's authenticated deposit history.

Native vote lookup searches retained blocks and these ancestor commitments.
Historical mandatory deposit proofs are reconstructed through authenticated tree
prefixes. Voting-period selection continues into the preserved ancestors instead
of falling back prematurely to the new storage anchor. Repeated pruning prepends
newly removed ancestors and preserves earlier pins.

Local snapshot v3 serializes the ancestor commitments and validates them during
canonical restoration. Existing v1/v2 formats remain for their prior profiles.
A native pruned snapshot without the required ancestry is rejected. The blanket
native-pruning refusal is replaced by these ancestry checks. There is still no
automatic scheduling or deployment in this tranche.

## Evidence scope

The deposit-history test executes real deposit-contract transactions, then builds
a synthetic Beacon ancestry over those execution headers and deposit histories.
It checks ten native period-vote comparisons, lookup of all five historical votes,
rejection of a foreign-branch vote, exact preservation of a mandatory 1,240-byte
deposit operation and repeated pruning. Altered roots, missing original ancestry
and reordered ancestors are rejected. This is a helper-level native qualification,
not a full native Capella state-transition or v3 snapshot restart test.

The Capella BLS/pruning regression separately checks real local signed blocks,
finality, durable pruning and RocksDB reopening under the existing non-native
profile, covering v1/v2 behavior after the storage changes.

## Remaining gates

A full native Capella run with v3 checkpoint restart, new deposits after pruning,
slashing proofs, failure injection and automatic scheduling remains required.
Ancestor metadata still grows linearly with canonical history, although without
full EVM/Beacon states. No indefinitely bounded storage, production readiness,
EIP closure or recovered disk-space claim is made. Compact on-disk ancestry
indexing and long-running measurements remain necessary.

Results: both tests passed (0.39 s and 34.02 s). Source/binary hashes captured before testing and verified afterward. Logs normalize trailing whitespace only.
