# HTTP read diagnostics, 2026-10-01

Five new tests pass: three distinct Beacon 503 messages remain fatal; successful Beacon and JSON-RPC error body semantics stay unchanged; malformed status/JSON fails closed and raw non-JSON error bodies are not echoed; diagnostics are bounded; real curl against a local HTTP server retains the 503 JSON body. run.mjs now captures status using curl --write-out and decodes it rather than discarding the body with -f. Transport failures remain fatal. No retry, timeout, consensus, recovery deadline or stability criterion changes. http-response.mjs is added to the campaign harness provenance ledger.

This is observer diagnostic qualification, not a correction of the underlying HTTP 503 cause or a passed WAN campaign. Runtime candidates remain unchanged.
