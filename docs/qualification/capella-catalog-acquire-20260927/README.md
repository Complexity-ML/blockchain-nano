# Request-addressed Capella inventory — 2026-09-27

## Protocol and resource invariants

The daemon obtains `nano_capellaCatalogGeneration(token)`, then calls
`nano_capellaCatalogAcquire(token, kind, generation, requestId)`. Both identifiers
are 256-bit random hex values. The client retains its request ID before sending.
`nano_capellaCatalogRelease(token, generation, requestId)` can therefore release
a snapshot even when the acquire response and server-generated handle are lost.
All three methods require the private bearer token. Holders share the existing
administrative principal; this is not public unauthenticated P2P state.

Acquire is idempotent for an active request in the current generation and rejects
conflicting kinds. Release fences a request even when it arrives before acquire.
Expired snapshots retire their request IDs. Retirement records do not expire
individually: when the configured handle-capacity bound is reached, the server
rotates the generation and clears the retirement records atomically. Old-generation
acquires are then rejected, including duplicate acquires of still-live handles.
Existing handles remain pageable; releasing an existing old-generation request
still finds and removes it. A fresh process uses a fresh random generation.

At most `maximum` live snapshots and `maximum` retired IDs are retained. The
existing aggregate root-memory budget and TTL still apply. Rotation does not
raise quotas, alter consensus, or delete an active snapshot. Generation races
can fail an attempt safely; the next relay attempt fetches a fresh generation.
There is no automatic retry of consensus delivery or acquire. Release retries
once on a transport-reported unknown outcome, under the overall relay deadline.

## Evidence scope

`source.patch` applies to the manifest base via `git apply --unidiff-zero`.
The manifest identifies modified sources and tested binaries. Test results and
commands are retained alongside this file. Tests use synthetic keys and no funds.

## Limits

If the core cannot be reached for cleanup, snapshots can still remain until TTL.
Cancellation cleanup is best effort and does not report successful completion.
This protocol cannot guarantee resource release on an unreachable process.
Legacy `nano_capellaCatalogOpen` remains supported and still relies on its returned
handle or TTL; the production daemon now uses the request-addressed API. A new
daemon needs the matching new core; there is no silent fallback to unsafe legacy
open. No new Docker image or VPS deployment is included in this qualification.
WAN partitions, malicious peers, high concurrency fairness and overall production
readiness are separate gates. Generation rotation under sustained concurrent
catalog traffic needs further liveness/load qualification.

## Completed local checks

- `build-network/nano-capella-catalog-relay-tests`: 20 scenarios passed, including
  a newly injected lost acquire response. Mocked ACKs establish relay state-machine
  behavior only.
- `ctest --test-dir build-release -R '^consensus_capella_wire_admission$'
  `--output-on-failure`: passed in 5.69 s. Real registry tests cover duplicate
  acquire, conflicting kind, owner isolation, release-before-acquire, replay after
  release/expiry/restart, bounded retirement rotation and old-generation release.
- `build-release/nano-capella-transition-tests --http`: passed. Existing loopback
  HTTP checks and the in-process real RPC/registry/relay integration run together.
  The new fault injector discards a successful acquire result, then checks release,
  unreadability and immediate quota reuse. It does not simulate TCP packet loss.
- `build-release/nano-capella-transition-tests --network-cli build-release/nano
  build-network/nano-p2p`: passed. Real libp2p, late follower, identical execution
  head/state and same-peer fresh-state recovery after a dropped ACK. This short
  case does not qualify distributed finality. Its relay logs include failed
  attempts with confirmed closure; not every catalog attempt succeeds.

The fixed legacy `runtimeCatalogChecks: 3` in the runtime summary is not the
number of assertions or coverage cases. Source assertions define this scope.

## Distributed regression

`build-release/nano-capella-transition-tests --distributed-cli build-release/nano
build-network/nano-p2p` exited 0. Two local processes, 32 synthetic validators
each, finalized the same epoch-2 root on three stable observations. Exact logs
are preserved in `distributed.log.gz`; summary and raw-log hash are separate.
There were 2,863 successful closed relay attempts and 8 failed but closed attempts.
This demonstrates bounded local finality progression after the protocol change,
not a new WAN, Docker partition or Byzantine qualification.
