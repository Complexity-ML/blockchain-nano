# Admitted-payload WAN attempt E — raw pass, strict deadline unqualified

Runtime 5165e51, harness c25cc31, three processes / two hosts / 64 synthetic keys, Capella/Shanghai, network tracing enabled. Both cuts ran; raw harness exited 0 and passed both cycles. Recovery records were 140,186 ms and 181,001 ms after verified reconnection. Strict common finality advanced 15→18 in 143,649 ms. All terminal validators/clocks healthy; Docker exits 0 without OOM. Data volumes retained.

The second record exceeds the nominal 180,000 ms limit. Inspection found the old harnais checked its deadline before the asynchronous sampling read, not after; logging before budget creation could also extend it. The exact physical finality time inside the sampling interval is not known. Preserve the raw pass but do not qualify strict <=180s recovery. The original partition module is retained with a hash verified against the campaign ledger. Corrected deadline tests/evidence are in recovery-deadline-20261001.

All captured quantum yields on all three nodes report 8..15 admitted payloads as designed; no premature quota violation observed. This verifies runtime quantum behavior, not causally proven speed improvement or production readiness. Prior WAN A/B causes and other production criteria remain open.
