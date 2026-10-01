# Bellatrix pruning clock barrier

The clean a00ff7d0 Docker ARM64 selection executed all 16 registered tests: 15 passed, the Bellatrix voluntary-exit pruning scenario failed. CTest returned 8. Sources and executable hashes were unchanged before/after execution. No tests were skipped.

All six previously failing RPC duration scenarios passed in this selection; successful and nested RPC cases, core, the load-window control and clock-coalescing negative controls also passed. This is bounded ARM64 evidence, not an AMD64 result or full production approval.

The failing observer reports nowMs=60000, observedMs=48000 and maximumForwardStepMs=6000: the synthetic clock advanced two slots without synchronizing the observer at the exit-inclusion slot. Commit b0eb5ba1 adds that synchronization barrier, conditional on a network peer being present. It does not change the clock limit or protocol rules. The barrier itself still requires linked execution qualification.

Raw logs are stored as gzip with mtime zero. The first launch omitted Docker interactive stdin and executed no tests; its empty log is retained, excluded from the actual CTest evidence. The subsequent launch executed all 16 cases.
