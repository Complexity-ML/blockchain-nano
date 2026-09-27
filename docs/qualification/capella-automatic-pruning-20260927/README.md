# Automatic Capella pruning — 2026-09-27

## Runtime change

The configured Capella runtime enables automatic pruning by default. The optional
local `automaticPruning` boolean can disable maintenance; it does not change the
consensus identity or fork rules. Raw reactor constructors remain manually
controlled for explicit maintenance and tests.

After a tick commits its normal clock/vote transition, maintenance considers the
finalized anchor. It waits until the required recent epoch context can be retained.
If a retained signed vote references a target or block outside the candidate,
maintenance waits for normal vote expiry instead of dropping evidence. Otherwise
it publishes the qualified pruning candidate through the normal checkpoint path,
including complete restart validation before writing. Failures propagate; uncertain
writes still fail closed. The tick and pruning are two separate durable commits.

## Qualification

`consensus_capella_pruning_automatic` enables maintenance on both native-profile
execution owners and runs 72 blocks with local signatures and votes. It records
anchor movements and the maximum number of retained block-state records, requires
multiple moves, preserves exact agreement between both owners, and reopens both
RocksDB databases at the end. Earlier mid-run restart/signing-journal checks remain.
The configured-runtime regression separately exercises startup/lifecycle with the
new default. No public node or VPS is updated by these tests.

## Limits

This bounded local scenario does not qualify the deferred foreign-branch vote
case, slashing-proof interactions, sync/power failure between the two commits,
WAN behavior or indefinite storage growth. Historical native deposit metadata,
execution history and archives still grow. Bounds observed for retained block
states must not be described as bounded total disk use. More failure and resource
qualification is needed before public production.

Results: runtime regression passed in 21.75 s; automatic pruning passed in 67.87 s. The native loop produced 72 blocks and 528 votes, finalized epoch 7, moved the anchor six times, retained at most 31 block states and ended with 18 states plus 55 historical deposit pins. Both owners reopened successfully. Source/binary hashes were captured before tests and verified afterward. Logs normalize trailing whitespace only.
