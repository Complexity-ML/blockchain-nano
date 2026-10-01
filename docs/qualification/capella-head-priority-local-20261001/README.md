# Capella head-priority relay: bounded local qualification, 2026-10-01

Source parent: 967a0dc2299da0a756e152c8d93e61dcfbfd3d5b. This archive records the uncommitted P2P patch by exact source hashes and patch; it does not label these binaries as the parent revision.

The block relay selects the coherent authenticated local head, delivers authenticated ancestry with receiver ACKs, then retains the historical catalog. Head and catalog share dependency quantum and one absolute deadline. A completion after that deadline cannot report success. No quorum, fork-choice, signing or WAN recovery deadline changed.

Final v2: 17 modeled scenarios passed in native and Docker ARM64. Both real distributed-cli runs exited 0: two daemons, 32 validators each, common finalized epoch 2 observed three times, healthy signers and clocks. Native head-priority completion events: admitted 57, cached 177, selected/unconfirmed 1. ARM64: admitted 61, cached 175, anchor 2. These are event counts, not unique blocks. Exact roots and admission counts are in the v2 summaries.

Adjacent regressions were explicitly rebuilt: 41 catalog, 25 block and 14 attestation scenarios passed on native and ARM64. Their sources did not change between wrapper v1 and v2. The first native regression attempt failed on filesystem permissions before execution; its log is retained. Earlier 15/16-model iterations and real v1 results are retained separately. A late-callback review led to the final deadline guard and new scenario, followed by fresh final real runs.

The private coherent-head core at exact parent revision also completed Docker AMD64 beacon-read and signer-health qualification (exit 0); its logs and binary hashes are included. No P2P head-priority AMD64 qualification yet. Existing runtime images and volume revision markers remain old; do not reuse their provenance for modified binaries.

No WAN partition, connected-majority island, sync inclusion/rotation, full lifecycle, load or production qualification is established here. WAN O remains failed at its original 180-second deadline. No improvement in WAN recovery is demonstrated. Archive integrity verification alone does not validate test semantics or source provenance. No private keys or database directories are archived.
