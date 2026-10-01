# WAN L — publication capture exceeds read budget, 2026-10-01

Source 72ea2ed, harness 989c85a. First recovery passes in 140184.833167 ms, finality 2→7, strict 180000 ms unchanged. After the second requested 192 s partition, VPS headers/head returns HTTP 503 with phase publication-pin and total elapsedMs 3102. Cancellation is observed immediately after current() captures the store under the admission mutex, before fork-choice projection. Total elapsed is wall time, not an exclusive mutex-wait measurement.

Three signers and clocks healthy at stop; Docker states/logs retained. Observer aborts before second recovery/stability qualification. Optional network/duty trace disabled in L, enabled in K: no single-cause performance comparison. No budget relaxation or production readiness. Databases, journals and witnesses retained separately; no keys archived. Previous failures remain open.
