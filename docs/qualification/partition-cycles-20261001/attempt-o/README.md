# WAN partition cycles O — 2026-10-01

Runtime `9513ee9f9f0aabc1a331e457e5437b867b793fb9`, harness
`eb3cf649de2fc5647201111da5d1385e9cfd2f8c`. Sources, images and budgets frozen
throughout. Existing reviewed ARM64/amd64 candidate images reused; no rebuild.
Three processes on two physical hosts, 64 synthetic validators (22/21/21),
Capella/Shanghai, 8 slots/epoch, 6 seconds/slot; Docker 2 CPUs/3 GiB,
nofile 65536. Optional network trace disabled. Finite cuts 96/192 seconds.

Campaign exit 1. First recovery passes: common finality 2→7 in
139500.768709 ms monotonic; complete final read 1721.296 ms.
Second reconnect at wall timestamp 1790851186414. The unchanged strict 180000 ms
window expires at monotonic 180001.60575 ms. Its last complete read, at
179243.467333 ms, still reports finality epoch 7 (`advancedCommonFinality=false`),
with read cost 3043.51575 ms. The 1.60575 ms overrun measures when the deadline
was reported, NOT the time needed to recover. A subsequent diagnostic window
sees epoch 15 in 1940.812708 ms; this is NOT the original recovery duration.
The first complete late read occurs at wall timestamp 1790851368360,
181946 ms after reconnect. Actual protocol transition time between reads is unknown.

Late stable finality 15→18 passes in 141746 ms, bound to finalized execution
blocks. All three clocks/signers healthy at stop; 418/399/397 attestation payloads
admitted. Both containers exit 0 without OOM. Resource summary contains 47 sampled
observations: it is not a capacity, throughput, disk-occupancy or peak guarantee.
Docker BlockIO is cumulative I/O, not database size.

During cuts all three have zero peers: this bootstrap-star scenario does not
qualify continued finalization of a connected majority island. Authenticated
block-dependency delivery before votes did not close this second recovery gate;
this single run does not establish a comparative timing benefit or its sole cause.
M/N and earlier failures are preserved. Inspect actual canonical vote inclusion
and branch convergence next; do not repeat unchanged campaigns for green or
increase deadlines. Historical fork choice/off-chain vote replay, other production
criteria, independent review, bridge and fair launch remain open.

Only public receipts/logs and harness sources are archived. Private keys,
fixture.env, signer journals/witnesses and databases remain outside Git.
