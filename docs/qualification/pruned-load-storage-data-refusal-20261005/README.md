# Pruned workload storage DATA refusal — 2026-10-05

Controller terminal **1**, harness 2d7ba2ee, runtime ab10b8ae. Three nodes, two physical hosts, 64 synthetic validator keys, private SSH-loopback, pruning and whole-node restore, explicit 250 ms fixture drift budget. No funds or public deployment.

Whole VPS restore and separate signer commitments accepted; common finality 2→3 in 78,972 ms under 180,000 ms. All 96 type-2 transactions accepted and their complete successful canonical finalized receipts independently verified equal across three nodes, anchor59 `0x85072798e13569aec7609374dc80274991e2f80e196562a4685a9c9c9f4d0f88`.

Fresh-state collector failed at 275,845 ms with `Invalid state quantity`: `eth_getStorageAt` is padded 32-byte DATA, not compact QUANTITY. Source `src/rpc.cpp` emits the proper word, while the test stub had `0x2a`. Correction enforces exactly 32 bytes, preserving quantity checks for nonce/balance. Reference: https://ethereum.org/developers/docs/apis/json-rpc/#eth_getstorageat .

No finalized state success, stable post-load window or terminal offline ancestry audit claimed. Terminal validators healthy, common epoch10 root `0xb54d29ac2407de62c009d33a94028f20e2761047827aebaaf4bb6547b399cdfc`; Docker containers exit0, no OOM. Native programs unchanged, frozen harness checked against Git2d7ba2ee. Data retained at `/home/ubuntu/nano-qualification-muuldkb5`. Archival path normalization error retained separately. Global production and independent review remain open.
