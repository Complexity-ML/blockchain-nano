# Repeated WAN partition campaign C — terminal failed

Runtime revision 10e2f9e, harness 9f4e344, packaged ARM64 and amd64 candidates recorded in clock-runtime-images-20260930. Three processes, two physical hosts, 64 synthetic validator keys, Capella/Shanghai, no funds or public network. Cuts requested 96 and 192 seconds. Recovery deadline remains 180 seconds.

Cycle 1 passed. Cycle 2 recovered after the deadline and remains failed. Late stable common finality advanced strictly from 15 to 18, with healthy terminal clock/validator receipts on all three nodes; both Docker containers exited 0 without OOM. The previous clock discontinuity was not reproduced; its cause and WAN A remain open. Runtime change only adds diagnostic fields and preserves all clock checks. No recovery fix was made, and no production readiness is inferred.

Full compressed logs/resources and sampled Beacon observations are retained. Timing analysis describes first observations, not continuous event timing or causation. Container states are recorded; data volumes are retained.
