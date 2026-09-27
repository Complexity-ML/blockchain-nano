# Capella reusable stream integration — bounded asymmetric-delay scenario PASSED

A versioned `/nano/capella/session/1.0.0` capability is advertised outside the
strict three-field consensus advertisement. Legacy peers retain single-message
streams. New peers use one sequential sender, at most 16 frames per stream,
5-second sender idle expiry, existing receiver 10-second frame deadlines and
unchanged per-message admission budgets. Each frame still receives its own
accepted/rejected acknowledgement after durable work. Unknown results are not
retried by the sender. Peer session/capability changes invalidate the sender;
inbound dispatch rechecks peer compatibility and generation per frame.

Eight new sender cases bring the stream suite to 38 cases: rollover at 16 frames
with a rejected ACK; malformed ACK; missing ACK; generation loss; failed write;
busy/stop/late opening; expired opening completing after a new successful request;
and idle expiry. Existing stream/timer tests and four compatibility/budget/relay
suites pass. Optional top-level capability compatibility is additionally checked.
Native P2P builds. These fake-stream tests do not replace real libp2p evidence.

The Docker source snapshot uses base 15f1b1864ff687ded3dd8eb8947eda1bf8193c7f
plus the accompanying patch, with an explicit dirty-source label. The additional
peer-status test was edited after Docker context capture; it is native test-only
and does not affect the daemon. No VPS or real-fund operation.

## Real network result

The unchanged `asymmetric-delay` harness exits 0. Two real libp2p daemons with
32 synthetic validator keys each reach common finality epoch 2 before the fault,
then epoch 4 while node 1 has 500ms egress delay. The delay remains active for
147.985 seconds (96-second minimum plus the finality observation gate). Netem
reports 565 affected packets, zero drops. After removal, common finality epoch 5
is qualified in 43.826 seconds, including repeated observations and header reads.

- Under impairment, epoch 4 root:
  `0x7e9b03e5a5794acc35f68de20235cf8c2c95a105d3c0bd765d52fdcd79d15e78`.
- After restoration, epoch 5 root:
  `0xe8e75d30bddfb0fccfd87925139b36a753669e0f8a8e5c8d9b56b2589fc08141`.

No conflicting finalized checkpoint is observed. Temporary head differences
remain visible in the complete event log. Both containers and their dedicated
network were removed; independent filtered Docker inventories were empty.
Public fixture keys are test-only and must never be funded.

## Transport measurement and limits

During impairment, median attestation send time is 1492ms on node 0 (79 samples)
and 1500.5ms on node 1 (68 samples), versus about 3s in the earlier serial
stream-per-message counterexample. Median stream-open phase is now 0ms on both
nodes (clock resolution), versus about 1.5s previously. ACK wait remains and
includes transport, admission and scheduling. Samples are grouped by completion
wall-clock time; boundary-crossing requests are included in their completion
phase. This comparison is descriptive, not a repeated statistical benchmark.

Full-run receive totals: 1787 accepted ACKs, 48 rejected ACKs and 1 ACK-write
failure. No inbound-budget refusal is logged. Two attestation sends have unknown
outcome across the complete run, one under impairment and one after restoration;
they are retained, not reported as accepted. Cleanup can contribute to final-run
unknowns; these logs alone do not attribute every unknown to cleanup.

This closes the specific two-node local 500ms scenario for this source/image.
It does not qualify arbitrary delays, geographic WAN, many-peer fairness,
validator scale, crash consistency, malicious partitions, or overall production.
Receiver idle reads retain ordinary ingress permits until reset/deadline; the
impact with more peers than available bulk permits still needs qualification.
Mixed-version compatibility is checked at advertisement level, not by a separate
mixed-binary network campaign. Historic failures remain in their own directories.

The exact source patch is zero-context (`git apply --unidiff-zero`), with hashes,
image provenance, logs, fixture and analysis retained here. No test deadline,
quorum, block rule or budget was relaxed.
