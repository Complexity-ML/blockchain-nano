# Physical journal rollback V2 — source only

Parent: immutable `14b4117c284c043d157e033af733f8fc5b4bab7a8f5bab24f4c61d90571d384a` (19,808 bytes). Only this new TU and README are proposed; no Core/helper/parent mutation, compilation or execution by the author. Suggested repository destination remains `tests/qualification/beacon_journal_physical_rollback.cpp`.

The single CLI argument is an absent absolute run root with an existing physical parent. It creates initial current P/W stores, closes all handles, copies both completely, reopens the same key/domain and signs one public proposal, then closes every mutable handle. A separately retained `commitments.json` beside `old/` and `current/` is EXCL-created, fsynced with its parent, mode 0400, held by FD9/known bytes/SHA before checking. Floors are read from that file, never inferred from the old stores.

This successor changes only admission of the **new cold copies before their first Seal**. Each new file retains its creation-FD eight fields (FD9 except ctime), captured after its last write/fsync, and the known source SHA. Directory device/inode/type/mode/uid/gid are anchored at creation; all eight fields are fixed at the first complete post-construction observation. Names/types/hashes remain fixed. All directory syncs finish before observation.

Three complete strict captures must agree across at least two seconds, within 30 seconds and the unchanged global 180-second budget. Only ctime differences between otherwise valid unadmitted observations restart the consecutive series. Any other field/hash/name/type change refuses immediately. Quiet comparisons here do not emit a helper failure for an allowed pre-admission ctime change. A capture that itself violates the existing helper’s strict checks is refused, not silently retried. The original source Seal is refenced before and after each complete sample.

The adopting Seal receives the exact last admitted vector and immediately fences it. It never recaptures a replacement baseline. No copied-file write, chmod, durability sync, copy retry or reseal follows admission. All subsequent old-store, current-store and commitment FD9/SHA checks remain strict, including ctime. The old Seal stays held across current-journal advancement and both RO passes. Failure JSON adds admission sample/streak/time, ctime-change and source-fence counters; the PASS record schema is unchanged.

Expected counts remain one provider/BLS, two producer signer constructors destroyed before checking, four stores, eight RO opens/two cold passes, seven Core authentications, four positives and three exact Core refusals (recent primary floor alone, recent witness floor alone, both). Actual record tokens stay distinct from floors. Bounds remain 32 MiB aggregate store payload, 4096 entries, 180 seconds, 128 MiB observed RSS; the caller must also enforce the process timeout.

Root reported Native04 on the parent as FAILED after two positives and three refusals: an old-copy file changed ctime with its other eight fields and bytes unchanged. Cause/actor are unestablished; a currently visible provenance xattr does not prove its prior absence. Root reported AMD02 PASS for that same parent. Neither result is reclassified. This successor needs its own qualification and may still fail any post-admission variation.

No operational restoration, duty authorization, common finality, signature-delivery history, digest ancestry, protection of the external authority against simultaneous rollback, or production readiness is claimed. No automatic CTest with a reused output directory is proposed. Sibling target proposal only:

```cmake
add_executable(nano-beacon-journal-physical-rollback tests/qualification/beacon_journal_physical_rollback.cpp)
target_link_libraries(nano-beacon-journal-physical-rollback PRIVATE nano_core)
target_compile_features(nano-beacon-journal-physical-rollback PRIVATE cxx_std_20)
```

Sources remain 0644 and unexecuted pending Root review/freeze.
