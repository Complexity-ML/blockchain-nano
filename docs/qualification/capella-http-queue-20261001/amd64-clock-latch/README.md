# AMD64 clock latch and HTTP admission qualification

Source: `74784c499cc904eba1d766933bc2017d3e2be9a0`. Three targeted Docker tests pass: deterministic HTTP admission/clock contention 26.93 s, four clock fault/cancellation diagnostics 0.46 s, queue expiry 1.22 s. Public RPC and Beacon must answer before release in unchanged 500 ms windows while private admission and the real clock poll wait.

The first AMD64 dispatch-only failure remains in ../amd64-first-attempt. The corrected fixture forces clock contention; the runtime health latch no longer waits for the poll mutex. Poll ordering, time jump limits and fail-closed invalidation remain intact. The P2P binary is reused with identical network/third_party sources and prior hash. This is targeted integration evidence, not WAN recovery or production qualification.
