# Capella send phase timing — measured latency counterexample

The serial asymmetric-delay counterexample remains open. The rejected two-vote
pipeline is not present. Only trace-enabled send diagnostics change:
`test-consensus-send-completion` records kind/peer, wall-clock completion time,
monotonic elapsed milliseconds at stream-open and write completion, total elapsed,
and accepted/rejected/unconfirmed outcome. Negative phase values mean that phase
was not observed successfully before completion. TimedReply emits once; logging
exceptions cannot change the delivery result. No deadlines or budgets change.

Phase differences separate stream negotiation from write and ACK wait. ACK wait
also includes receiver transport, scheduling and core work; it is not pure
network RTT. Sender pacing/catalog waits before send_altair are excluded.

Native build and timed_reply/altair_stream checks passed. Docker images built successfully; campaign ended with exit1 (finality deadline).
Next: identical500ms one-peer egress scenario, archive successes and failures,
then compare per-phase distributions before/during fault. This is diagnosis,
not a finality fix or production qualification. No funds or VPS modifications.

## Measurements and conclusion

Finality again stopped at epoch3, root
`0xd2adf53edeb9fae42d9fce039ea3fbac3c85e2512ebe9653ff8cac4b26127645`.
No production fix is claimed. Under the unchanged500ms impairment:

| Attestation sender | Samples before / after fault start | Median total before | Median total after | Median open after | Median remaining after write |
| --- | --- | --- | --- | --- | --- |
| node0 |105 /46|213ms|3019ms|1507.5ms|1512.5ms|
| node1 |103 /45|219ms|2893ms|1429ms|1512ms|

Write duration median is0ms. The node1 open/write distributions have44 samples,
as one completion had no successful open. After-fault outcomes: node0 has39
accepts/7refusals; node1 has41accepts/3refusals/1unconfirmed. All remain in logs.
Samples are grouped by completion wall-clock time relative to fault start;
this includes boundary-crossing sends and cleanup. Monotonic clocks measure
individual durations. `analysis.json` contains counts, median, p90 and maxima.

Repeated stream opening and per-message response waits impose measurable cost.
Combined with a serial catalog this limits fresh-vote throughput. Remaining
post-write time includes receiver scheduling/admission and transport, so these
measurements alone do not assign it exclusively to network RTT or prove the
sole cause of stalled finality. Next implementation should amortize negotiation
and acknowledgements with bounded reuse/batching, preserving per-message
admission outcomes, byte/work bounds, cancellation and unknown-outcome semantics.
Increasing deadlines or removing admission limits is not the proposed fix.

Artifacts retain complete logs and synthetic fixture; test keys are public and
must never be funded. No post-removal finality qualification was reached. This
instrumentation is enabled only by existing test tracing; no payloads or secret
keys are added to diagnostic events.
