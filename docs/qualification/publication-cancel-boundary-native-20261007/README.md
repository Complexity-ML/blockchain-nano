# Durable publication cancellation boundary — native draft

V37 failed with a cancelled Capella message and an unavailable reactor. Source inspection found a cancellable finalized cold-record lease after durable witness intent had been written. This draft resolves the finalized block and performs the last caller cancellation check before witness_prepare. I/O failure handling remains fail-closed.

New regression monitors the actual independent witness checkpoint token during child admission. On the current parent core it throws `cancellation callback after durable witness intent` (exit1); with the draft core the same regression passes (exit0), followed by the existing cold checkpoint/restart checks. Single core translation unit and the new fixture compiled; parent library reused unchanged, draft archive replaces only that object. This is a native scoped comparison, not a clean release, Docker qualification, WAN root-cause proof, or production qualification.

The initial parent execution exited by signal6: exception unwinding destroyed the backing owner before the borrowing reactor. Preserved in initial/. The repaired fixture destroys the reactor explicitly before propagating the exception, yielding the intended parent1/draft0 comparison. No failed run was reclassified as passing.

Production sources are unchanged. ARM64/AMD64 qualification, full post-V37 storage audits (including failed VPS without a terminal signing receipt), and common-finality advancement remain open. No network node was restarted for this regression.
