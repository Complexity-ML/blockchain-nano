# Capella receiver attribution — 2026-09-27

The optional stream observer reports one terminal outcome on the I/O executor.
It distinguishes predecode budget rejection, unavailable peer, framing/decode
failure, unavailable dispatcher, core local failure, deadline/cancellation,
and successful/failed ACK writes. Observer exceptions are contained. Lease lifetime,
wire ACKs, admission limits and consensus rules are unchanged. A successful write
is not proof that the sender received the ACK. Frame-read-failed groups framing,
size, transport and read-timeout outcomes; it is not a specific network diagnosis.
The daemon emits this trace only with its existing network trace setting.
Kind is null before decoding; numeric kinds follow AltairWireKind (0 block,
1 attestation, 5 sync message). Handshake/eligibility resets before this handler
are outside these counters. No payload, secret or new network endpoint is logged.

## Tests and observations

Four targeted CTests pass (0.57s). Stream tests add seven cases: accepted/rejected
ACK, failed write, held peer admission capacity, unavailable dispatch, local core
failure, and throwing observer. Duplicate completion and cancellation after finish
must not duplicate the terminal report. Observer failure must not alter admission.
Existing cancellation/deadline tests continue to verify worker lease retention.

The bounded two-node real-libp2p test passes with 32 disjoint keys per node,
common finalized epoch 2 observed three times and eight checked finalized blocks
(slots 9–16), each with 32/32 sync positions. BLS and shared ancestry are verified
by the existing runner. This remains one physical host and the initial committee.

Receiver: 169 inbound-budget rejections before decoding; 979 accepted sync ACK
writes, 11 rejected sync ACK writes, 205 accepted attestation ACK writes and 62
accepted block ACK writes. No core-local-failure, ACK-write-failed or handler
deadline was observed. Sender: 132 sync and 29 attestation delivery-unconfirmed
attempts; 11 sync rejections. Sender catalog attempts and receiver streams are
not a one-to-one metric (block dependency delivery can use more than one stream).

This establishes actual admission pressure, not a per-message explanation of
every unknown outcome. The budget label does not distinguish rate, concurrency,
peer-table or global capacity. No claim of packet loss or absence of other faults.
Next: differentiate those admission causes and bound/schedule outbound attempts
without weakening inbound protection. Rotation, WAN, adversity and production
qualification remain open. No VPS deployment, funds or public launch.

## Reproduction

`ctest --test-dir build-network -R '^(altair_stream|altair_core_dispatch|capella_catalog_relay|capella_block_delivery)$' --output-on-failure`

`build-release/nano-capella-transition-tests --sync-distributed-cli build-release/nano build-network/nano-p2p`

Source patch is relative to the base commit in provenance.json; source/binary
hashes were checked after execution. Full logs retained with trailing whitespace
normalized only. summary.json contains exact counters and sampled block roots.
