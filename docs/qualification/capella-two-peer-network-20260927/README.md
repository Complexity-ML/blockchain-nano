# Capella real two-peer catch-up — 2026-09-27

Base revision `57aef0c` plus accompanying changes.

The daemon now connects CapellaCatalogRelay and CapellaBlockDelivery to private
core RPC and authenticated libp2p streams. It rotates block/attestation/slashing
catalogs under the existing per-peer/global relay permits. Receiver ACKs follow
actual core worker admission. Block ancestry comes from the authenticated local
archive. Catalog close remains allowed after peer disconnection for cleanup.

A public, non-secret capellaSession marker accompanies the compatibility profile.
Changing it or the core gossip epoch replaces the per-peer knowledge generation.
An attempt checks that generation before sending and when the ACK returns; late
responses cannot populate the replacement cache. Catalog completion cancels any
still-active dependency delivery before releasing its permit.

Activation is opt-in through the runtime network section, disabled when absent:
```
"network": {
  "enabled": true,
  "pageEntries": "0x8",
  "maximumEntries": "0x400",
  "dependencyBlocks": "0x100",
  "dependencyBytes": "0x1000000",
  "acknowledgedRoots": "0x400"
}
```
These are this local test's resource budgets, not production sizing or validator
count limits. Admission and catalog services are required. Recovery verifies that
the relay settings and authenticated anchor remain stable across its session
rotation. The core reports networkAdmissionEnabled as configuration intent;
networkProtocolActive is null when enabled because only the daemon can observe
registration. The daemon IPC-ready event records actual protocol activation.

Qualification command:
```
build-release/nano-capella-transition-tests --network-cli build-release/nano build-network/nano-p2p
```

The test starts two real nano supervisors and nano-p2p processes, listening only
on loopback. The source holds 64 synthetic validator keys and first produces at
least two blocks. A keyless late follower must reach the same execution block
hash and state root. The follower then stops and returns with a fresh store and
witness, reusing exactly its prior P2P identity. This is a deliberate test reset,
not an operational rollback procedure. One accepted message's ACK is dropped;
the fresh follower must still catch up. Peer identity equality and the dropped
ACK event are asserted. The final observed common height/hash/state root appear
in the capella-catchup-match record; all processes must shut down successfully.

The unchanged private-runtime and managed-node scenarios were checked after
wiring, as were shared IPC recovery and both relay unit suites. Their logs are
retained. Per-attempt negative relay results can occur during retry or shutdown;
the claim is eventual bounded catch-up in this scenario, not zero retries.

Limits: validators are concentrated on the source. This is not distributed
validator finality, independent-host/WAN qualification, partition healing, large
registry scaling, sustained load or adversarial consensus qualification. Finality
has not advanced in this short test. Catalog expiration/pruning under long catch-up
and fair relay scheduling under large inventories remain to qualify. No VPS,
public network, user funds or bridge deployment were touched.
