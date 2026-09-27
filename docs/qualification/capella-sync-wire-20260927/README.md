# Sync message SSZ and bounded ingress — 2026-09-27

## Implementation

The standalone codec encodes exactly 144 bytes: little-endian uint64 slot,
32-byte Beacon root, little-endian uint64 validator index and 96-byte signature.
It follows the [SyncCommitteeMessage field order](https://ethereum.github.io/consensus-specs/specs/altair/validator/#synccommitteemessage).
The decoder rejects every other length; semantic validation remains in the
reactor. `sync_message_ssz` decodes before locking, then selects the current
checkpoint and admits atomically under the reactor lock, avoiding a current-token
read/admission race in the worker adapter.

A new explicitly gated envelope kind, `sync_committee_message`, is supported by
the bounded hex decoder, compact base64 IPC decoder, core ticket dispatcher and
Capella worker. Full chain/genesis/fork/consensus identity remains required.
The daemon enables the capability only in Capella mode. Older profiles keep the
flag false and reject the type. Existing private RPC session/token handling and
queue/budget limits are reused; no public method bypass is introduced.

## Qualification

A fixed golden vector checks SSZ field order and endianness. The local-sync loop
now sends individual messages through the real Capella service receipt/worker
adapter, alternating hex JSON and compact base64. It checks accepted completion,
replay through SSZ admission, malformed sizes in both encodings, wrong chain and
wrong consensus identity. The complete independently computed sync aggregate
must still appear in the proposal; two owners execute/finalize the blocks.

The core-dispatch unit test gains a gated 144-byte sync case, short-message
rejection and legacy-profile rejection. Existing fragmented-stream regressions
are rerun. The nano-p2p binary is built with the new ingress kind.

## Limits

This proves codec, service/receipt admission and daemon dispatcher behavior in
local tests. The dispatcher uses a fake RPC responder; the native loop invokes
the same worker adapter used by private RPC directly, without an HTTP socket.
No new live libp2p/WAN sync exchange is claimed. Outgoing sync gossip, retention/
export of ephemeral messages, local validator signing and scheduler timing remain
open. SSZ compatibility does not imply Ethereum gossip/subnet compatibility.
Large-registry per-message costs and collection across reorgs remain unqualified.

## Result

Passed: native service/SSZ scenario 38.76 s (40 blocks, 272 attestation votes,
finalized epoch 3, two owner reopens); dispatcher and fragmented-stream tests
0.52 s combined. Nano core test binary and nano-p2p build succeeded. All captured
source/binary hashes were unchanged after execution. Logs normalize trailing
whitespace only; existing compiler warnings are retained.
