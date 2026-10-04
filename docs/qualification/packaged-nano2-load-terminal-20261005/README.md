# Packaged finite load — terminal success, 2026-10-05

Original controller/campaign exited **0**, no restart or deadline change. Actual Mac native + Docker ARM64 + VPS Docker AMD64, three processes/two physical hosts, 64 public synthetic validators. Same immutable Nano2-capable runtime/images; **pruning disabled explicitly** for this default storage-profile workload.

96 signed type-2 transactions, synthetic fixture key/account, finite requested ramp at 1000/500/250 ms intervals. All 96 admitted, mined in blocks 34–44 and proved successful/canonical/finalized with complete receipts identical across all nodes at finalized block 47. Sender nonce, 96 wei target balance and storage slot 0 = 42 matched. Workload including finality/receipt collection took 276,123 ms under unchanged 600,000 ms limit; not execution latency or throughput. Measured ramp rates retained in scope.json, not assumed to equal requested rates.

Post-load common finality **8→11**, all validators stopped healthy. Both Docker containers exited 0 without OOM. Immutable executable hashes and native programs rechecked; recorded receipts reverified with the real receipt verifier. Harness unchanged.

31 usable resource samples per node, zero unavailable samples. Max observed memory: native main-process RSS 302,907,392 bytes (P2P child excluded), Docker ARM64 350,434,099 bytes, Docker AMD64 290,770,125 bytes. Docker limits 3 GiB / 2 CPUs. CPU, sample gaps and cumulative BlockIO retained; memory accounting differs between native and Docker, peaks between samples unknown, BlockIO is not disk occupancy.

This closes the scoped nominal finite workload/inclusion/state-agreement check, not maximum capacity, sustained saturation, complex DeFi, pruning under load, endurance or public mesh. Historical WAN A was not reproduced by this campaign; its original cause remains unknown. No real funds, sale, public deployment, continuous flood, recompilation or image build. All wider production and independent-review criteria remain open.
