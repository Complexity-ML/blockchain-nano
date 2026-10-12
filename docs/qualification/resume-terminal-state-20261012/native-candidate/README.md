# Fresh durable native candidate — 12 October 2026

Previous temporary native artifacts are unavailable. A fresh Release build of
`nano`, `nano-capella-paused-capture`, and `nano-capella-transition-tests` completed
from clean source revision `0842078e253e42424ef554d4c7662cd09d8305ea` in this durable
workspace, outside `/private/tmp`. Production C++ remains equivalent to
`85eb492f641452c4746ee60f027eacefa775b0b0`. No Docker image was rebuilt.

Original controller `71193` exited 0, terminal receipt `90a1f7`. CMake configure
exit 0 (3.66 seconds), selected target build exit 0 (137.42 seconds), parallelism
2, Apple clang 21.0.0, native arm64. Complete tracked compilation-input hashes
were identical before/after. Binary identities are in `binary-manifest.json`.

Because these are newly rebuilt binaries, three relevant CTest registrations
were checked exactly, then executed once: clock diagnostics, published signed
inventory, and durable-publication cancellation boundary. Original controller
`19016` exited 0, terminal receipt `5c4936`; 3/3 passed in 12.66 seconds, all four
binary/library hashes unchanged. The log and JUnit report retain per-test times.

This proves only this build and the selected checks. No live native node,
V37 signing duty or P2P network was started. Current V37 restoration, full release regression, cross-architecture
reproducibility, new EIP status or production readiness is claimed. Current Mac
signing journals remain unavailable; earlier network failures remain failures.
