# Concurrent Capella catalog qualification — 2026-09-27

## What ran

New CTest `consensus_capella_catalog_concurrency` invokes the native C++ runner
with `--catalog-concurrency`. It uses a real Capella durable reactor and nonempty
block snapshots. 32 OS threads run 32 barrier-synchronized waves. Each wave holds
all admitted snapshots until every admission attempt has returned, then releases
all requests, including refused ones. A fixed synthetic clock prevents expiration
from concealing resource leaks. No public network, funds or continuous flood.

Two configurations both permit four handles: root budgets of 576 and 1,152 bytes
hold respectively two and four snapshots of this fixture. Their observed peaks
were exactly two and four. Respectively 64/128 acquisitions succeeded and 960/896
were refused for capacity. All 32 workers subsequently acquired, read and released
one snapshot in an explicitly serialized recovery schedule.

32 concurrent identical requests returned the same handle under a one-snapshot
quota. 128 acquire/cancel races left no readable handle or replayable request;
127 acquisitions won the race, one cancellation won it. Every race was followed
by a fresh full-size snapshot allocation proving quota reuse without TTL.
The consensus checkpoint was unchanged. The surrounding signed-transition fixture
also passed. Total CTest time: 5.92 s, exit 0, 120-second timeout.

## Finding: fairness remains open

In the two-snapshot contention run, two workers received zero admissions across
32 waves (see `perWorkerAdmitted`); the four-snapshot run admitted every worker at
least once. The registry mutex and capacity rejection do not promise fairness.
Quiet serialized recovery demonstrates lack of a persistent quota leak, NOT
starvation freedom under continuous contention. No fairness assertion was removed
or relaxed to get this result; the test records per-worker distribution explicitly.

The daemon already rotates peers using `upper_bound(altair_last_peer_)` in
`poll_capella_archives` and enforces one active attempt per peer. That production
scheduler is NOT exercised by this registry test. Next qualification should use
multiple competing peer relays with delayed replies, generation rotation and
small catalog quotas, and check bounded per-peer progress. Do not translate 32
threads into 32 nodes, validators or a distributed production load claim.

## Provenance and scope

Only test code, CTest registration and documentation changed in this tranche.
No production runtime fix or VPS deployment. The manifest records source/binary
hashes during the run, rechecked after completion. The patch applies to its base
using `git apply --unidiff-zero`. `concurrency.log` is the complete verbose CTest
output and `summary.json` extracts its concurrency result.

This adds bounded evidence for resource limits, duplicate requests, cancellation
races and recovery. It does not close WAN, consensus, full EIP, bridge, operator
security or independent-review production gates. Thread scheduling is inherently
variable; exact per-worker counts and race winners are observations of this run.
