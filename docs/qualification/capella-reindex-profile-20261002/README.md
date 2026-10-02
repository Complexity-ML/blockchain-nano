# Reindexed activation CPU diagnostic — pending

Original uninstrumented campaign completed 9/10 with unchanged manifests.
A single diagnostic replay of its failed reindex case was gated on this terminal
state. First attempt exits runtime 1 on missing /work/tests/fixtures manifest,
wrapper 0 after unchanged binary/library manifests; no CPU sample acquired.
Controls and failure retained. Corrected B mounts frozen source 3030b799 readonly
and uses a distinct scratch directory. Same binary a125a741..., original
360-second bound, 2 CPUs/3 GiB and 30-second cpu-clock:u sample at 49 Hz after
slot 96 is reached. Process 491325 was confirmed active. Not a qualification
or a before/after performance benchmark; raw perf stays private on VPS.

## Terminal corrected native sample

Attempt A lacks /work fixtures and stops before sampling; runtime exit 1,
wrapper 0 after unchanged manifests. Attempt B mounts frozen source correctly.
Its automatic PID selector mistakenly picks the idle timeout supervisor and
produces no usable native data; this output is retained separately. The actual
C++ process 491325, matching expected program hash, is sampled for 30 seconds
without relaunching or extending the existing test. 1,419 samples, zero lost;
raw perf remains private on VPS. The corrected selector is recorded, not rerun.

Exclusive leaf costs include JSON dump_escaped 5.07%, malloc 5.00%, keccak 4.58%,
hex encoding 3.10%, along with cryptography and unresolved addresses. Stacks
include Capella durable_snapshot/store serialization. Nested percentages must
not be added; no before/after comparison or total speedup inferred. The next
optimization must preserve exact complete-record equivalence and all validation
and cancellation guards while avoiding repeated conversions on memo hits.

Diagnostic runtime exits 124 at the original 360 seconds; program and libraries
before/after match, manifest exit 0, no OOM. Its instrumented result earns no
qualification and never replaces the uninstrumented campaign's failure.
