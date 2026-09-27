# Capella asymmetric packet delay — failed

Status: FAILED, exit1. Production binaries were built from clean commit
`c3696938febcc63bbb015f5ffb7f7b28511a799e`. The qualification-only derived image
adds iproute2. The harness grants NET_ADMIN only to the isolated test containers;
`tc` runs as root inside one container, never on the host or VPS.

New opt-in mode: `asymmetric-delay`. After common finality epoch>=2, apply
500ms egress delay to node1 eth0 only. RPC probes use container-local loopback.
Both P2P daemons remain running. Hold the impairment for at least96seconds,
require common finality >= baseline+2 while impaired (144s extra deadline),
verify netem transmitted packets, remove it and require another finalized epoch
within240seconds. Preserve monotonically increasing finality and reject
conflicting finalized roots. Three successful observations and finalized headers
are required for each common-finality gate. Cleanup removes only named campaign
resources; failures cannot count as success.

TCP ACKs from the delayed node are also delayed: this is one-sided packet delay,
not independent unidirectional application-message latency. Single Docker host,
64 synthetic keys in two groups; no funds, public deployment or continuous flood.
Does not qualify packet loss, Byzantine votes, WAN or large-scale operation.

Reproduce after building the base image:
```
docker build -f tests/capella/Dockerfile.netem -t blockchain-nano:capella-netem .
node tests/capella/docker-partition.mjs build-release/nano-capella-transition-tests /private/tmp/nano-asymmetric-campaign blockchain-nano:capella-netem c3696938febcc63bbb015f5ffb7f7b28511a799e asymmetric-delay
```
Output directory must not already exist. All exporter keys are synthetic,
publicly known test keys and must never be funded.

## Counterexample

Baseline common finality epoch2 succeeded. Netem reported delay0.5seconds at
10:48:50 UTC. Both nodes advanced to finalized epoch3, root
`0x52abb57aee8fc226ae499300b6a611d0f539346c5bc9d2b715bf80bd02831102`,
but never reached required epoch4 in the96second hold plus144second wait.
Heads diverged while both processes/clocks remained healthy. No conflicting
finalized root was observed; this is failure of bounded finality progression,
not evidence of double finalization. Recovery after removing the delay was not
qualified because the under-impairment gate failed first.

The harness removed the fault and deleted its two containers, volumes and
network; independent Docker inventory confirmed no resources for campaign
`nano-capella-partition-2e4d1643d7e2`. Full logs/fixture are under `failed/`.
All64 fixture private keys are publicly known synthetic test keys: never fund.

Receiver totals:128 block accepts,304 attestation accepts/6refusals,
990sync accepts/34refusals; no observed budget rejection or ACK-write failure.
Core sync refusals:21root mismatches,10already-reached inclusions,3expired slots.
Attestation catalog completions near shutdown took roughly3–27seconds.
These observations point to propagation/backlog investigation; they do not
establish the precise cause. Preserve deadlines and consensus thresholds.
Next: trace serialization and dependency delivery of fresh votes under latency,
fix proven causes, then replay this unchanged fault and regression scope.

The netem packet-counter success assertion was not reached. Its configured
qdisc was observed repeatedly but final transmitted-packet statistics were not
captured on this failed path; do not claim that measurement passed.
