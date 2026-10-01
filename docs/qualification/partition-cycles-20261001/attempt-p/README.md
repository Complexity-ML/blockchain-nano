# WAN P: head priority still misses the second recovery deadline

Runtime 86af5f8; harness cddea6c. Campaign process exited 1. Three processes on two physical hosts, 64 synthetic keys (22/21/21), Capella/Shanghai, slots 6 s, epochs 8 slots. Existing verified packaged images; no optional network trace, nofile 65536, Docker 2 CPUs/3 GiB. Finite cuts 96/192 s; original monotonic recovery bound 180 s unchanged.

First cut: common finality 2→7 in 137847.111625 ms, complete read 1739.578375 ms: qualified. Second cut: deadline report at 180245.186708 ms with last complete read still epoch 7 (read cost 1718.301 ms). This is the time of failure observation, NOT successful recovery time. Last read reports advancedCommonFinality=false. Late epoch-15 observation arrives 190589 ms after reconnection by wall timestamps; its 10342.188708 ms monotonic duration belongs to a NEW late window, not the original recovery window. Late stability 15→18 in 137729 ms passes.

All three signers and clocks healthy at stop; admitted local attestations 423/399/395. Both containers stopped exit 0, without OOM. Sources stayed frozen; no pulls or builds during campaign. Every island loses all peers during the cut; this does not qualify a connected majority island. Resource samples are descriptive; BlockIO is cumulative I/O, not disk occupancy; native RSS excludes its P2P child.

The head-priority correction does not close the second recovery gate. No causal benefit or production readiness is established. Prior O/N/M and other failures remain retained. Next: stopped-state canonical replay and epoch-14/15 vote inclusion/identity analysis. DB directories, private fixture.env, key material and private journal files are excluded.
