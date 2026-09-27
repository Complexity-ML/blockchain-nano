# Capella BLS catalog and real libp2p relay — 2026-09-27

## Implementation

Pending BLS withdrawal changes expose SHA-256 identifiers through live inventory,
stable catalog snapshots and private export. Inventories are sorted and only
announce operations applicable to the selected head. Retained signed bytes remain
exportable until finalization removes them; a snapshot never pins payloads or
prevents finality cleanup. The existing root, envelope and export budgets apply.

The daemon rotates four categories per peer: blocks, attestations, slashing
proofs and BLS changes. BLS acknowledgement caches are bounded and cleared on peer
session/generation changes. The catalog relay requires matching profile, message
kind and SHA-256 of the exported signed operation before delivery, and requires
successful consensus admission before acknowledging. Cancellation and catalog
release continue through the existing request-fenced lifecycle.

## Qualification

- `catalog.log`: native durable queue scenario, including root-budget and
  payload-byte-budget rejection, equality of live/stable inventories, exact
  exported signed bytes, restart, proposal inclusion and later withdrawal.
- `relay.log`: 22 catalog relay scenarios, including new valid BLS delivery and
  an incorrect advertised identifier. The latter must close the catalog without
  calling delivery. These are orchestration tests with mocked acknowledgements;
  they do not establish consensus validity by themselves.
- `libp2p.log`: `nano-capella-transition-tests --bls-network-cli
  build-release/nano build-network/nano-p2p`. Two real native cores and real P2P
  daemons run on loopback. One core controls 64 synthetic validators; the other
  has no validator keys. A test-only child shim uses its existing supervisor IPC
  credential to admit one signed change solely to the non-validator core, retires
  the receipt, then execs the real daemon with the same arguments. No production
  authentication bypass, printed token, modified daemon or injected block is used.
  The producer must receive the pending operation through the catalog/libp2p
  path, propose it and pay a subsequent withdrawal. Both peers must observe a
  positive balance at the initially empty requested address. The follower is
  then replaced with fresh state under the same peer identity; a lost reply is
  injected and common execution head/state must recover.
- `initial-libp2p.log` preserves the first passing run before the additional
  export-hash check. Final artifacts are identified by `provenance.json`; source
  and binary hashes were captured before the final tests and checked afterward.

## Limits

This qualifies local transport and the live producer path, not distributed-stake
finality, WAN behavior or Byzantine safety. Only one pending operation is relayed
in this network case. Reorg re-selection, queue saturation, large catalogs,
pruning of admission contexts and interrupted durable admission retain dedicated
open gates. The four-category scheduler has not yet repeated the earlier
six-node stalled-peer campaign. No public activation, VPS update, funds or full
EIP-4895/production qualification is claimed.
