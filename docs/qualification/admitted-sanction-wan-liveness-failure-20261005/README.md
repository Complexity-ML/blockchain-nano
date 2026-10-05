# Admitted-member sanction WAN: terminal liveness failure

The bounded two-host run ended with exit 1 and FAILED after its deadline. Both hosts signed duties (native: 600 attestations, 73 proposals; VPS: 537 attestations, 74 proposals). Durable finality remained at epoch 75. No controlled capture or crash phase ran.

The hosts built competing branches with different epoch-170 target roots. A bounded read of 20 immutable native block bodies and exact BLS verification attributes all 32 sampled epoch-170 single signatures to synthetic indices 0–31. This does not prove a global absence of VPS votes, a stake quorum, or the root cause of propagation failures. Source logs contain stale peer latest messages, catalogue timeouts and rejected deliveries. Status mutex contention is an investigation candidate, not a proven cause.

Original logs and databases remain in the source output directory named in result.json; their log hashes are retained. Committed logs omit large duty traces, with terminal summaries and bounded diagnostic evidence retained separately. The attribution binary is a read-only temporary diagnostic linked against the recorded existing runtime library; this is not a new runtime qualification. Independent review and all production gates remain open.
