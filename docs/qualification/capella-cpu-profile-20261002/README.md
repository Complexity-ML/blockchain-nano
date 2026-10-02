# Capella CPU observation — parent candidate, suite still running

Thirty-second sampling of the already active AMD64 process 461994 running
`--deposit-auto-registration`, source cac8d719, unchanged original 360 s timeout.
No extra test run, no runtime change. cpu-clock:u at 49 Hz, DWARF 8192-byte stack,
1,403 samples, zero lost samples. Raw perf.data stays on VPS; path and SHA-256
are in provenance.json. Observed executable SHA-256 matches the campaign's
before-tests manifest. Whole-suite terminal manifests are still pending.

Leaf samples show JSON escaped-string serialization (6.77%), JSON lexer (4.49%),
keccak (4.42%), malloc (3.28%), and hex encoding (2.49%), among other symbols.
Stacks include Capella durable_snapshot and CapellaDurableReactor::publish.
This window supports removing repeated canonical parse/dump and copies, but
cannot attribute all JSON cost to one function or predict total speedup. Nested
stack percentages must not be added. Unknown addresses remain unresolved.

The source sampled is the parent of the serializer candidate 3030b799; this is
not a before/after benchmark. Two earlier suite cases timed out; the observed
log is explicitly in-progress, not a terminal suite report.

Independently, integrated ARM64 candidate activation passed in 90.94 seconds,
queued BLS changes passed in 1.99 seconds; restart and remaining cases are still
running. No AMD64 qualification of the new serializer or production claim.
