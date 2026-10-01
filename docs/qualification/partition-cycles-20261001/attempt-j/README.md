# WAN J — reproduced fatal Beacon read failure, 2026-10-01

Frozen runtime 67c4103 and diagnostic harness 59ac33e. First partition recovers common finality 2→7 in 136153.374292 ms, within 180000 ms. After the second 192 s partition, the VPS headers/head read returns HTTP 503 with body message "Beacon state unavailable". The observer aborts before a full recovery/deadline outcome or stability qualification. It is the general execution-exception branch, not the explicit read-capacity or queue-expiry response. The old runtime still combines cancellation/budget expiry with other exceptions here, so the exact underlying exception is not known.

Three validators and clocks are healthy at stop; Docker states and full public runtime/observation logs are included. Data, witnesses and signing journals remain retained separately; no key material is archived. No WAN speedup or production readiness is claimed. I and all previous failures remain preserved.
