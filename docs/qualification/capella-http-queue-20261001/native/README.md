# Capella HTTP worker dispatch — native scope

Baseline test fails (21.19 s) at the check that a pending real Capella status RPC must not stall public HTTP. It holds the real admission mutex via authenticated signed-block export, issues a private HTTP status request, and checks eth_chainId plus Beacon finality responses before releasing the lock.

After adding all 14 Capella private methods to the existing bounded worker classifier, the same scenario passes (20.45 s): both reads complete before release, private request remains pending, checkpoint unchanged. Notifications reserve one slot and oversized private batches hit capacity. Existing RPC queue expiry regression passes (1.84 s), including HTTP/WebSocket, notifications, wallet work and capacity retention. Worker count, capacity, queue/Beacon deadlines and WAN recovery limit unchanged.

Initial test compilation rejected mixed auto declarations; corrected, error log preserved. No runtime admission, signature, finality or storage rules changed. Docker ARM64/AMD64, real daemon transport and WAN effect remain unqualified. T and S failure evidence preserved; no exclusive cause of T timeout claimed.
