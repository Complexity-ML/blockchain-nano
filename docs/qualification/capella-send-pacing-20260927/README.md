# Capella send pacing — 2026-09-27

## Changes

The actual consensus ingress uses InboundBudget::Traffic::bulk: peer refill
4 requests/s, burst 8, concurrency 2, in addition to shared global/control
limits. Earlier references to the general peer rate of 8/s did not describe
this narrower bulk admission. None of these receiving limits were increased.

Capella catalog sends now share a peer-local monotonic pacer across all kinds
and block dependency sends. Up to four requests may start immediately after idle;
thereafter starts are spaced at 250ms. Existing one-catalog-per-peer ownership
bounds queued work; no per-message unbounded queue is introduced. Deferred sends
check peer generation and catalog lifetime before opening a stream. Existing
catalog deadlines remain. Reconnection does not reset the pacer of a retained
peer entry. Peer deletion creates a fresh local pacer; receiver budgets remain.

This is local outbound shaping, not receiver-capacity negotiation, global
multi-peer scheduling, gossip aggregation or an Ethereum subnet implementation.
Other bulk traffic still competes for receiving tokens. Arrival jitter and other
senders can still exhaust receiver budgets. High RTT still limits throughput.

Budget rejection diagnostics now distinguish invalid identity, peer-table,
peer/global rate/concurrency and their bulk subsets, preserving first-failing
check order. A null kind before decode remains unknown. String literals describe
the failed check; no untrusted payload or secret is logged.

## Qualification

A deterministic clock test checks 1000 scheduled sends against the unchanged
receiver, initial and post-idle bursts, and backwards-clock spacing. Admission
cause tests cover invalid peer, peer bulk concurrency/rate and propagation through
the stream observer. Existing budget tests preserve global/control reservations.
Five targeted CTests passed; final run and hashes are recorded alongside this file.

The strict-spacing experiment is retained in strict/: its real two-node run
passed common finality 2 and eliminated observed receiver budget rejections,
but sampled participation was [28,32,32,32,32,32,32,31] across slots 16 down to 9.
Two sync deliveries remained unconfirmed, 20 were explicitly rejected, and three
exports were unavailable. This exposed a delivery-timing tradeoff and motivated
the bounded burst. Strict spacing is not the final implementation.

Each source.patch is a delta from the provenance base for tracked files; its
adjacent capella_send_pacer.hpp supplies the new file at network/capella_send_pacer.hpp.
Source/binary hashes are checked after runs. Logs normalize trailing whitespace
only. Separate runs use different genesis times and are not a controlled
statistical benchmark. These are two local peers and one physical host, not WAN,
committee rotation, scale or production closure.

Reproduce: build nano-p2p and the five targeted test binaries; run
`ctest --test-dir build-network -R '^(inbound_budget|altair_stream|altair_core_dispatch|capella_catalog_relay|capella_block_delivery)$' --output-on-failure`, then
`build-release/nano-capella-transition-tests --sync-distributed-cli build-release/nano build-network/nano-p2p`.

## Final bounded-burst result

Five targeted CTests pass in 0.49s. Two real local peers reach common finalized
epoch 2 for three observations. All eight finalized samples at slots 9–16 contain
32/32 sync positions, with independently verified BLS and shared block ancestry.
Receiver: 953 accepted sync ACK writes, 18 rejected sync ACK writes, 207 accepted
attestation ACK writes and 62 accepted block ACK writes. No budget rejection,
local core error or ACK-write failure observed. Sender: one sync delivery-unconfirmed,
18 sync admission-rejected, zero sync export-unavailable; 199 sync catalogs complete.

Relative to the previous unpaced run: budget rejections 169 -> 0 and unknown sync
outcomes 132 -> 1; full participation retained in these eight samples only. This
is bounded evidence, not a reliability rate or guarantee. The 18 explicit sync
rejections and one unknown outcome remain open: slot/head churn is a hypothesis,
not established attribution. The unchanged 210s finality deadline passed.
Next: attribute those core refusals and qualify rotation/asymmetric networking;
retain aggregate admission protection for larger peer populations. No deployment,
funds, public launch or production readiness claimed.
