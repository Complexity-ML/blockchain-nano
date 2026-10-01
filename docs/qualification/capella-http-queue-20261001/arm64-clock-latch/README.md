# ARM64 clock latch and HTTP admission qualification

Source: `74784c499cc904eba1d766933bc2017d3e2be9a0`. Three targeted Docker tests pass: queue expiry 1.03 s, four clock fault/cancellation diagnostics 0.16 s, deterministic HTTP admission/clock contention 8.00 s. The public RPC and Beacon deadlines remain 500 ms; the real poll and private request remain pending while admission is held. No consensus deadlines changed.

The P2P executable is reused from qualified 5bd6f95: network and third_party source inputs are identical (empty diff), and its recorded hash matches the prior candidate. Core is rebuilt. These tests do not qualify WAN recovery or production. Prior AMD64 dispatch-only failure and WAN S/T failures remain preserved.
