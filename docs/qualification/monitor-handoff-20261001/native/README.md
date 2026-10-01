# Durable monitor-to-delivery handoff — native

35 passed, zero failed/skipped, including four new integration cases. Real child-process SIGKILL after monitor/outbox commit, verified stopped-child lock recovery, replay of fault before new healthy observation; receiver failure queues durable retry; identity/corrupted state refusal; symlink/path-lock collision refusal.

RPC endpoints and receiver are synthetic local HTTP servers, not live Nano nodes. Tests do not qualify blockchain SIGKILL, physical power loss, real notification destination or full production readiness. Node v25.9.0; ARM64 uses the previous read-only temporary official runtime and existing builder, 2 CPUs/512 MiB, network none. Exact source hashes are ../source-hashes.json; base revision 329a7e4bd3f3b34d72b7621da6e1573ff3a274ce.
