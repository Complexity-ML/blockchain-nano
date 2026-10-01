# Connected majority island R — strict bounded gate failed

Runtime 418aceb8, harness a994bed. Terminal exit 1. Three processes on two
physical hosts; existing ARM64/amd64 packaged candidates, 2 CPUs/3 GiB each
Docker node, nofile 65536. No runtime rebuild or source edits during the run.
Capella/Shanghai, 6-second slots and 8-slot epochs, 64 synthetic validators.
Hash-bound SSZ effective balances give Mac native + Mac Docker
1,376,000,000,000 of 2,048,000,000,000 gwei (>2/3); VPS 672,000,000,000.

A direct private loopback P2P link kept the Mac nodes mutually connected while
the dedicated VPS network was disconnected for requested 192 seconds
(193241 ms wall observation including restoration/reads, not a monotonic
recovery duration). VPS zero peers observed, original IP restored even on
failure. Majority finality advanced 2→4; isolated VPS stayed at 2. Last
majority head slot 64, justified 6, finalized 4. The strict gate required
finality >5 to exclude pre-cut received commitments; it failed. This neither
proves a complete stall nor qualifies newly earned majority finality under
this budget. Recovery and three-advance stability were not executed after
the failed gate. No conflicting finality or regression was observed; this
is a bounded observation, not an unlimited safety proof. All signers healthy
(local admitted attestations 159/153/155); containers stopped exit 0, no OOM.

Native read-only canonical audit matches the independently captured stop
checkpoint: 54 blocks including trusted anchor, 143296 archive bytes, one
transaction. Inspector verifies 180 individual BLS signatures in epochs
4–7. Epochs 5/6/7 each contain all 43 majority keys, no VPS keys, but only
38/35/39 were included before the next epoch boundary. The other 5/8/4
were included at slots 48/56/64. Source epochs 4/4/5 respectively. These
observations explain a narrow timely quorum shortfall; they do not isolate
a sole relay bug, disprove eventual finality or justify changing quorum.
The full canonical audit validates inclusion separately. Historical
standalone votes and historical fork choice are not replayed. Inspector
uses existing core libraries; only its epoch filter changed from 12–15
to 4–7. This is an offline analysis executable, not a network build.

47 pertinent harness tests passed before the campaign (13 new). Archive
excludes databases, fixture.env, keys and private journal/witness files.
Public initial SSZ/profile, ownership, exact receipts, stopped states,
source snapshots, diagnostics and sampled resources retained. Resources
are descriptive; native RSS excludes P2P child, Docker BlockIO is cumulative
I/O rather than disk occupancy. The initial warm-up curl connection refusal
is retained. An extra read-only /eth/v1/node/health probe returned 404; it
is not used as a success signal or a gate.

Next: sustained connected-majority coverage across additional epochs to
distinguish delayed finality from a permanent stall, preserving this failed
192-second criterion and unchanged 180-second recovery limit. No production
claim, EIP upgrade, bridge, scale or adversarial-load inference.
