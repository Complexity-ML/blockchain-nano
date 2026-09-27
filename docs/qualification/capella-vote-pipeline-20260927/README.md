# Bounded vote delivery pipeline — failed integration qualification

Counterexample: `../capella-asymmetric-delay-20260927/README.md` remains open.
Serial catalog delivery waits for a remote ACK per message; observed attestation
catalog durations near shutdown were3–27seconds under one-peer500ms egress delay.
This motivates a bounded pipeline; it does not prove the sole cause of stalled
finality or that pipelining fixes it.

Candidate change: allow two exports/deliveries in flight for attestation and sync
message catalogs. Blocks and their dependencies remain serial. Existing sender
pacing, inbound concurrency/rate budgets, catalog deadlines and consensus rules
are unchanged. Never advance a page until every message in it is acknowledged.
On rejection/cancellation, late completions cannot resume the pipeline.

Native build passed. Three targeted suites passed (stream lifecycle, catalog,
block delivery), including three new cases: out-of-order ACKs and page barrier,
rejection with late ACKs, cancellation with late ACKs. The catalog suite has36
scenarios. Inbound budget/pacing regression also passed. Mock ACKs do not prove
consensus admission. Docker images built successfully, labelled source-dirty=true. The unchanged
asymmetric-delay campaign exited1: common finality deadline exceeded.

Required next gate: same `asymmetric-delay` harness, same500ms impairment,
96second hold and144second finality deadline, then240second post-removal gate.
No production readiness claim, no VPS change, no real funds.

## Result: insufficient correction

The unchanged500ms scenario failed again: common finalized epoch3, not the
required4. Root `0xbd3fa74519293acb347822dfbcd35be79168ea249a8ed05f711482a1a9ef7d01`.
The native unit checks pass but the production symptom remains. No conflicting
finalized root was observed. Do not deploy or describe this candidate as fixing
asymmetric-delay finality. Keep the original serial counterexample too.

Full logs and synthetic fixture are preserved under `failed/`; never fund these
public test keys. `analysis.json` retains admission outcomes and slow catalog
completions. The post-removal finality gate was not reached. Next investigation:
per-peer catalog scheduling and freshness/dependency delivery of votes; one active
catalog still blocks all other kinds on that peer. This is a code observation,
not yet a proven sole cause. Do not increase consensus timeouts to hide failure.

The candidate also introduced peer-bulk-concurrency refusals (see analysis),
absent from the serial baseline. Its production source and test edits have been
removed from the working tree; `source.patch` preserves the complete experiment.
Native network binaries have now been rebuilt from restored source;
`restored-binaries.json` identifies them separately from the rejected candidate. This failed candidate must not be shipped.
Campaign cleanup was independently verified: no named containers/network remain.
