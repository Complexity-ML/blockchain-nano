# Capella sync rotation during transport outage

Status: PASS (exit 0), scoped two-peer qualification.

New opt-in mode `--sync-rotation-outage-cli` combines the real two-peer
transport outage and authenticated rotated-committee verification. Each peer
holds 32 distinct keys; preset, six-second slots and transport limits unchanged.
Wait for a common head at slot >=120, require suspension before slot128,
SIGSTOP both child P2P daemons for96seconds while cores continue. Require both
isolated branches to cross128, diverge, and not finalize past the pre-outage
epoch bound. Resume; require finality beyond the highest observed recovery head
epoch. After clean stop reopen both witnessed stores, compare committed SSZ
states, prove committee differs from genesis and independently verify >=3
finalized aggregates containing remote positions in that authenticated period.
Only inclusion slots strictly above max recovery head +1 qualify. This cutoff
excludes old messages but does not timestamp signature generation relative to
SIGCONT. Participation percentage is measured, not presumed complete.

The existing long rotation recovery deadline remains unchanged; startup waits
for the real target period instead of shortening consensus parameters.
This is a transport-process outage on one host, not asymmetric packet loss,
physical disk failure, WAN, or a large validator network. No real funds or flood.

Reproduce:
`build-release/nano-capella-transition-tests --sync-rotation-outage-cli build-release/nano build-network/nano-p2p`

Build: PASS. First sandboxed attempt exited 1 before networking because local
bind was denied; see `sandbox-bind-failure.log`. Identical binary relaunched
with local networking permission. The subsequent run passed (details below).

## Observed result

Base `445b58b3979a3ca4c75b2da3f182c8996bb8b4d8` plus `source.patch`.
Source and binary SHA256 hashes verified before and after execution.
Both peers finalized epoch18, root
`0xdad758447a575c2564c184260ef4956b3442ec39b6e0c52620188b42005f3851`.
The two witnessed stores reopened with identical committed SSZ state at slot144.
The new committee hash is
`0xdf6b4506b6079a1d8bc7eb4577c604266a4d533d818d0fa0cf5f44eaad7e581f`,
different from initial
`0x65958d612a4fe6a024e4efb1be78b0cebf8b1796dc5b94fb3e3d4038a2641934`.
Suspension began with both heads at120; before resume the heads were136 and133.
The permitted finality bound during isolation was epoch15.
The recovery inclusion cutoff was137. Independently verified samples:

| Inclusion slot | Committee positions /32 | Remote positions |
| --- | --- | --- |
|139|12|0|
|140|32|12|
|141|32|20|
|142|32|12|
|143|32|12|
|144|32|20|

Five samples qualify as remote; slot139 does not. No claim of immediate full
participation. Both clocks and validator schedulers reported healthy at shutdown.

Receiver sync:4433 accepted ACKs,67 refused ACKs,one ACK-write failure.
Core rejections:56 root mismatches,11 already reached inclusion slots. These
are early rejection causes, not proof of cryptographic validity. No budget
refusal or local-core transport failure observed. Sender sync:2 unconfirmed
deliveries,6 unavailable exports,67 explicit refusals. One attestation refusal
and2 block-catalog refusals are also retained. Catalog successes can include
already-known entries; counters are not unique delivered messages.

Artifacts: `run.log`, `build.log`, `summary.json`, `provenance.json`, `source.patch`.
The sandbox bind failure is preserved separately and is not a consensus failure.
This closes this bounded scenario only. Asymmetric packet loss, independent
hosts/WAN, crash recovery under load and large-population qualification remain
open. No new EVM/EIP-wide or production-ready claim.
