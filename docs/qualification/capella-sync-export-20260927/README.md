# Capella ephemeral sync message export — 2026-09-27

## Implementation

Accepted individual sync messages are retained by validator index for one proposal
slot and selected parent. The committee size bounds their count. Prepared map
allocations precede aggregate publication; cancellation cannot publish the message
alone. Duplicate messages remain exportable without duplicating entries, including
members already covered by a submitted aggregate. Nothing is written to RocksDB.

Pinned inventory snapshots expose sorted SHA256 message IDs; the live inventory
API deliberately refuses this ephemeral kind because a durable checkpoint token
cannot describe changes to this cache. Export rechecks current slot and selected
parent and enforces payload limits. Old pinned roots may therefore expire before
export, which aborts and releases the relay attempt.

The daemon polls a fifth catalog kind with its own bounded acknowledgement cache,
reset on peer session changes. The catalog relay validates exact wire SHA256 before
delivery. This is Nano's private relay protocol, not Ethereum sync subnet gossip.

## Evidence

- Release C++ transition target and nano-p2p / catalog relay targets built.
- `consensus_capella_local_sync`: passed in 41.36 s. 40 blocks, 272 votes,
  finalized epoch 3, two independent RocksDB owners, two reopens, 64 journals.
- New checks: pinned snapshot stability, unique committee member count after
  replay, root memory budget, refusal of legacy live inventory, exact export
  digest, 144-byte payload bound, admission on second reactor, unavailable export
  and empty new catalog after selected head changes. Durable token unchanged by
  collection. Existing malformed SSZ, signatures and fork-domain checks retained.
- `capella_catalog_relay`, `altair_core_dispatch`, `altair_stream`: 3/3 passed
  in 0.14 s. Relay adds successful sync export and wrong-digest rejection; its
  transport and ACKs are mocked, not consensus evidence.
- Initial build failed only in the new test's string/JSON comparison. It was
  corrected to an explicit string conversion; the initial compiler log is retained.

## Remaining gates

No real sync libp2p/WAN exchange or HTTP socket was exercised here. Local validator
signing/scheduling, delivery before proposal deadlines, subnet selection,
reorg/restart during collection and large-committee cost remain unqualified.
Slot expiry is enforced in code; the new expiry assertion specifically covers
head change. Retained expired memory is bounded and replaced on the next valid
context, not proactively freed at each tick. Map preparation copies up to one
committee's messages; performance remains to be measured. No production closure,
public launch, VPS deployment or real funds.

`provenance.json` records source and binary hashes before tests, rechecked afterward.
`source.patch` is the tested source delta from the recorded base. Logs are normalized
only for trailing whitespace. Commands used: `cmake --build build-release --target
nano-capella-transition-tests -j 4`, `cmake --build build-network --target nano-p2p
nano-capella-catalog-relay-tests -j 4`, and the CTest selections listed above.
