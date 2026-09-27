# Capella bounded wire admission — 2026-09-27

Base revision `1a4995e` plus accompanying source changes.

## Implementation

`CapellaConsensusService` reuses the checked Nano envelope decoder and ordered,
bounded Altair worker. The Capella domain commits chain ID, execution genesis,
trusted Beacon anchor and complete consensus profile. Full identity plus compact
fork digest must match before a message reaches the worker. Supported messages:
signed blocks, attestations and attester slashings. Voluntary exits remain explicitly
unsupported on this new service. This is Nano framing, not Ethereum gossipsub.

The runtime accepts optional `admission` capacities: frameBytes, blockBytes,
attestationBytes, slashingBytes, jobs and payloadBytes, all positive canonical hex
quantities. Capacity validation precedes signer construction/start and ensures the
configured profile's maximum messages fit. Clock polling precedes message handling.
Stopping the runtime cancels/drains admission before exporting checkpoint receipts.

Queue limits count queued plus running jobs and payload bytes. Local resource
budgets are not validator registry limits. The eventual authenticated P2P reader
must independently bound fragments and retain its per-peer ingress lease until
completion; no peer identity is taken from the JSON frame.

## New evidence

Wire scenario: 17 dispatched messages (16 valid and one invalid signed block),
including eight successive blocks, a sibling, six committee votes and a slashing
proof. Two reorgs still select the expected paired EVM state. Wrong chain, genesis,
fork digest or full identity, missing identity, unknown kind, unsupported exit,
duplicate JSON keys and oversized frame do not enter the queue. An invalid signed
body cannot change the checkpoint. A blocked one-job worker refuses a second job;
stop/cancellation prevents publication. A one-byte payload budget refuses admission.

The configured runtime produces 16 blocks around witnessed restart and retains
its exact network identity. Its admission callback accepts an archived duplicate
without changing the checkpoint. Undersized admission is refused before opening
any signing journal. The stopped runtime refuses further admission.

## Limits

Transport-independent in-process workers only. No sockets, libp2p protocol
registration, private RPC receipt interface or daemon inventory/sync is wired for
Capella yet. The `/nano/capella/1.0.0` identifier is declared, not activated on the
public network. Live mixed-fork Store migration and exit/BLS-change/sync queues
remain open. No continuous flood, real funds or production activation.

## Results

Initial two-case gate passed: configured runtime 21.35 s and wire admission
4.94 s (26.30 s total). After moving capacity validation before signer startup
and adding negative configuration/stopped-admission checks, only the changed
configured-runtime case was rerun: passed in 21.42 s. It reports 16 blocks,
64 loaded keys, five invalid configurations rejected and witnessed restart.
The unchanged wire scenario is covered by the initial log, not a second run.
Source and binary digests are recorded in `manifest.json`.
