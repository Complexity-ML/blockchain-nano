# Clock fault latch: deterministic native HTTP regression

First AMD64 attempt retained in ../amd64-first-attempt: identity ready, Beacon not ready, private request pending, checkpoint unchanged. Code revealed healthy() waiting for clock poll mutex, whose poll waits for admission.

Clock fault latch is now atomic acquire/release; poll ordering and observations still serialized. Backward/oversized steps still latch failure and invalidate reactor; cancellation still does not latch. Healthy indicates last latched fault, not fresh progress or finality. No timeout, worker/capacity or consensus rule changed.

Test now stops autonomous fixture polling and explicitly enters a real poll of the same runtime clock while holding the real reactor mutex; both remain pending before release. Identity and Beacon both complete within unchanged 500 ms windows, private RPC remains pending, checkpoint unchanged. New deterministic case passes 18.83 s; four fail-closed clock diagnostics pass 0.99 s; queue expiry regression passes 1.88 s. Docker qualification and WAN effect still pending.
