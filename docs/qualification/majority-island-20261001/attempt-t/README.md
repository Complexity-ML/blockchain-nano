# WAN T — majority passes, Beacon read timeout after reconnect

Runtime 5bd6f95 (historical export), harness 9b694ef; two physical hosts, three processes, 64 keys. Same 384 s cut and 180 s recovery policy as S. Common initial finality 2. Connected majority advances beyond ceiling 5 to epoch 8; isolated VPS remains at 2. Original Docker network address restored.

Campaign exits 1: after 30,993 s wall time from reconnect a captured immutable-root Beacon finality read on VPS hits curl's 5 s limit with zero bytes. This is neither a 180 s deadline failure nor a successful recovery time. Stability not run. At clean stop Mac nodes finalize 9, VPS finalizes 3: partial progress, no common recovery qualification. Healthy signers/clocks, both containers exit 0 without OOM, native PID absent.

Code inspection finds Capella private RPC methods missing from simulation_item: HTTP executes these calls synchronously on its event loop, including newly costly historical export and operations acquiring the admission mutex. This is a concrete scheduling gap; exact exclusive causation of T's timeout is not proved. No changed deadline, manufactured ACK, relaxed finality or unchanged campaign rerun. Prior S and all production gates preserved.

Only public stop commitments, public fixture/state, source snapshots and logs are archived. No database, private key, env, signing journal or recovery witness.

Stopped native and VPS canonical histories replay successfully, matched to their external stop commitments; snapshots opened read-only. This does not replay historical fork choice or standalone votes. First native audit request included an unsupported maximumBytes field and was refused before replay; corrected request and refusal retained. Private database copy stays in /tmp, not Git.
