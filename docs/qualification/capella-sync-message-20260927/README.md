# Individual native sync messages — 2026-09-27

## Implementation

`CapellaSyncMessage` carries the signed slot, Beacon block root, validator index
and fixed 96-byte signature. `submit_sync_message` requires the expected durable
checkpoint and accepts the message during the next proposal slot only. It rejects
slot overflow, unavailable/future slots and a head already at the proposal slot.
The selected parent is advanced under the authenticated Capella rules. The index
must exist, its public key must occur in the current sync committee and the
advertised root must match the authenticated previous-slot block-root entry.
The signature is verified with the appropriate fork/genesis sync domain.

For each matching committee position, admission adds it only if the current
slot/head cache does not already contain it. The single signature is repeated
algebraically for the number of missing positions, preserving committee
multiplicity. The existing verified-aggregate admission path merges this disjoint
contribution and validates the result. A valid already-covered message succeeds
without another contribution; wrong signatures never bypass validation on replay.
The reactor mutex spans context validation and cache update. No durable state or
validator signing journal is modified by receiving these signatures.

## Qualification

The local-sync scenario alternates empty blocks, blocks built from two aggregate
halves and blocks completed from individual messages after one aggregate half.
Repeated committee members are taken from the real slot-advanced fixture.
The assembled body must equal the independently computed full aggregate.
Individual messages are replayed immediately. Wrong root, overflowing/future
slot, unknown validator, corrupt signature, a valid signature from an outsider,
stale checkpoint and cancellation are checked. Both owners execute the blocks
and retain the existing finality/reopen assertions.

## Limits

Message admission is a local API, not P2P gossip or a public endpoint. Wire
encoding, network routing, validator signing and scheduler generation remain
open. The collection window currently requires the following proposal slot;
previous-slot precollection needs explicit scheduling work. Each message derives
and validates the state context again; large-registry throughput and context
caching must be measured before claiming scaling to 100k/1M validators. No such
scale or WAN qualification is inferred from this 64-validator fixture. Branch
switches and restart between individual messages remain separate tests.

## Result

Passed in 38.33 s (38.34 s total CTest): 40 blocks, ten full aggregates completed
with individual messages, ten built from aggregate halves, twenty empty blocks,
272 attestation votes, finalized epoch 3 and two database reopens. All negative
controls and equality assertions passed. Source/binary hashes were verified
unchanged afterward; logs normalize trailing whitespace only. Existing compiler
warnings are preserved. This is new individual-message coverage within the
existing local-sync test, not an additional independent network deployment.
