# Fresh GitHub Docker core compilation — success

Run 36887129414, source ee565c18cdea9f53625370f0b6c9f3cf66b4772f, completed/success. Core job 14m21s, Ubuntu 24.04 manifest digest 008173c23f95b170204355c12626cb5a965d779a7e1283b09e9cffbb1bf33ca3, 2 CPU/3 GiB, Clang 18.1.3. Fresh GitHub checkout; no existing Nano image or build cache used. Dependencies installed within the pinned Docker container.

All nine required CTest registrations present; JUnit verifies nine executed/passed, no failures/skips. Tests 26.56 s: core, chain, RPC queue expiry, BLS verification/fast aggregation, execution checkpoint, Capella paired payload/state, signed transition and clock diagnostics. Nano and the selected test executables compiled. Operations job independently successful. Artifacts downloaded from GitHub; original failed/cancelled attempts retained separately.

No cpp-libp2p build, distributed network, exhaustive EEST or complete validator lifecycle in this job. Apt versions/runner are not snapshot-pinned and binaries were not compared bit-for-bit: no reproducibility or complete production claim. Runtime images remain the separately qualified 74784c4 candidates. Upload-artifact v4 runtime deprecation annotation retained in workflow state; update of that CI-only action requires its own workflow check, no C++ source retest implied.
