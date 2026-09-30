# Clock diagnostic runtime images — 30 September 2026

Both existing Docker build volumes were incrementally built for nano and nano-p2p from Git revision 10e2f9e2c618cea7e96f36abc9114812fb1beec1. The revision marker was written only after both targets succeeded. Runtime assembly reused the prior immutable runtime images without dependency downloads. Packager checked executable hashes, shared libraries and nano --help; subsequent probes confirmed the installed hashes and identity. Native nano and Capella fixture exporter were rebuilt; the four actual-clock fail-closed cases passed natively as well. Prior ARM64/amd64 test evidence remains in capella-clock-diagnostics-20260930.

The P2P hashes are unchanged. No clock guard was relaxed. These images add diagnostics; they do not resolve campaign B, explain WAN A, qualify network recovery or establish production readiness. A fresh bounded partition campaign is required to observe the new diagnostics in the runtime.
