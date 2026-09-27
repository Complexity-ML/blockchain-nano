# Capella cleanup after peer loss — 2026-09-27

## Defect and correction

The daemon's peer-validity gate allowed `nano_capellaCatalogClose` after a peer
was disconnected or changed session, but rejected the new request-addressed
`nano_capellaCatalogRelease`. An acquired snapshot whose response was lost at
that point could therefore retain its quota until TTL despite the local core
remaining reachable. Registry/relay tests alone had not exercised this gate.

The gate is now extracted as `capella_peer_core`, used by `poll_capella_archives`
and the real RPC/registry regression. It permits both cleanup methods after peer
invalidation. Generation reads, acquire, page, export and submit remain blocked.
Core bearer-token authentication and registry request/generation validation are
unchanged; permitting a local cleanup call does not bypass core authorization.

## Verification scope

The configured-runtime test opens a real catalog through the production gate,
invalidates the peer immediately after the core allocates the snapshot, and
returns an unknown transport outcome to the relay. It then checks exactly one
forwarded release, failure of the interrupted relay with confirmed closure,
unreadability of the old handle and immediate full-snapshot quota reuse.
Additional checks ensure five new-work methods are not forwarded after loss.
The invalidation/lost response is injected in process, not a real packet drop.
The HTTP test also retains its existing real loopback HTTP and restart checks.

The two-process `--network-cli` regression covers actual libp2p late-follower
catchup and same-peer fresh-state recovery with an injected delivery-ACK loss.
It does not itself reproduce the precise disconnect-during-acquire race, nor
qualify distributed finality, WAN failures or scheduling fairness.

The previous 32-thread finding remains open: some workers can be denied access
through all contention waves. The daemon's rotating peer scheduler still needs
qualification with multiple competing slow/fast peers and generation changes.
No VPS rollout, Docker rebuild or public-fund operation is part of this tranche.

The manifest records source and binary hashes, rechecked after execution. The
patch applies to the recorded base with `git apply --unidiff-zero`. The runtime
summary's legacy fixed `runtimeCatalogChecks` count is not a coverage count;
the assertions and test source define the verified scope.

## Results

- `ctest --test-dir build-release -V -R '^consensus_capella_private_http$'`: passed,
  22.78 s. Includes the newly guarded invalidation/lost-open scenario.
- `build-release/nano-capella-transition-tests --network-cli build-release/nano
  build-network/nano-p2p`: exit 0, matching execution head/state, late follower
  and same-peer fresh-state recovery after ACK loss.
- Native transition runner and `nano-p2p` rebuilt successfully. Existing macOS
  deprecated `posix_spawn` warning remains. No consensus-rule change.
