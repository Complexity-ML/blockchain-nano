# ARM64 runtime packaging — 2026-10-05

Actual candidate assembly and network-disabled, read-only execution succeeded. Nano and nano-p2p hashes match the qualified binaries. UID/GID 10001, all eleven license hashes, dynamic-library closure and `nano --help` checked. No node or public network started.

The original assembly failed because only four notices were copied by the packaging path. Seven notices have now been added; the packaging test validates the real complete checksum manifest. The original failure log is retained. Only the affected package/export tests were run.

The standalone runtime base avoids rebuilding C++ and libp2p. The Ubuntu OCI index is pinned; installed apt versions are recorded, but apt repositories remain mutable and a fully hermetic dependency rebuild is **not** claimed. One base and one successful candidate image were produced; the failed attempt is preserved.

Compilation provenance is mixed and explicit in scope.json. Runtime sources are byte-equivalent to 59fa6530 and main 4eb31d3a; the image does not claim all objects were rebuilt at main. AMD64 assembly and subsequent multi-host qualification remain open. CI 37242291266 operations succeeded; core/network were still running at observation.
