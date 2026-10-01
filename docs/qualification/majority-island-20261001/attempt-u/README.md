# WAN U — connected majority, bounded recovery and consecutive finality pass

Runtime 74784c4, harness 3ab9c16; native Mac, ARM64 Docker Mac and AMD64 Docker VPS, two physical hosts and 64 synthetic validator keys (22/21/21). Candidate image IDs and both executable hashes verified before fresh fixture generation. Source frozen throughout. Capella consensus / Shanghai execution only.

Initial common finalized epoch 2. During the unchanged 384 s WAN cut, the two mutually connected Mac peers own 1,376,000,000,000 / 2,048,000,000,000 gwei (>2/3). They finalize through epoch 8 beyond the pre-cut ceiling 5; the isolated VPS remains at 2. Original Docker address restored. All three then agree on finalized epoch 10 and its root after 105915.175209 ms (monotonic, limit 180000 ms). Immutable-root Beacon reads continue to answer; no curl timeout abort as in T.

The subsequent stability observer preserves its original 288 s absolute deadline and all conflicting-root/regression checks. Common-finality jumps 11→13 and 13→15 reset the consecutive window without credit. Three consecutive advances 15→16→17→18 then pass in 266875 ms, with finalized execution hashes matched across all nodes. Both container exits are 0 without OOM; native PID is absent; all validator/clock stop statuses healthy.

This closes this finite connected-majority reconnection scenario for this candidate, not complete production qualification. Historical S/T failures remain preserved. No sole-cause claim, no manufactured ACK/checkpoint, no time/resource limit increase, no full historical fork-choice replay. Sustained asymmetric faults, validator lifecycle/scalability, broader regression/operations and bridge/economic launch gates remain as documented. No funds or public launch.

Archive includes public fixtures, stop commitments, source harness, observations and logs only. No private keys, env files, databases, signing journals or recovery witnesses.
