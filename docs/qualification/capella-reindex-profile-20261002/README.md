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
