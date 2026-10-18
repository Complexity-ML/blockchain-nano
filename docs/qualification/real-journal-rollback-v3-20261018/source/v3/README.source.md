# Real physical journal rollback V3 — source proposal

This uncompiled, unexecuted successor is limited to preparing the two states freshly produced by this test. Parent TU `b6760c6ddf98ec1cbd8968351e37155848d5eab752c4c11e2c0bf96b8bb901d6` (24,367 bytes), previous packages and their results remain intact. The existing helper `d3e2ddd78187e3c7994a709558e569a841838c487b9d266501306bca14dbc125` (20,319 bytes) is unchanged. Suggested destination remains `tests/qualification/beacon_journal_physical_rollback.cpp`; no Core change or production change is proposed.

The parent creates a strict `Seal` before `sync_tree` for both the initial and the advanced current pair. This ordering makes the subsequent sync part of the period already asserted immutable. V3 removes those two early Seals. It does not identify the actor or filesystem mechanism behind any observed metadata change.

Root reports Native successor04, session24299, as globally FAILED: its journal refused a source WAL ctime change before signing, Core authentication or cold-copy admission. Its independent physical produce/check CLI calls both returned zero (eight databases, five archives, fourteen public test signatures during produce, none during check), while the newly copied held manifest also changed ctime at the final controller fence. Neither success changes that whole failure; the held-manifest controller issue is separate and is not fixed by this C++ proposal. The older 14b AMD qualification and all Native failures remain historical results of their exact sources; none qualifies this V3.

## State machine and immutable boundaries

1. Initial P/W production closes all RW/signing handles. `admit_produced(...,"initial")` takes the first complete **unadmitted** capture: its names, types, eight stat fields (FD9 except ctime), and SHA values are fixed. This is a closed-producer snapshot, not a claim to OS-creation timestamps. It fsyncs all files and directories by relative FDs before any stable sample or Seal.
2. Complete post-sync captures must retain the same eight fields/SHA. Only ctime variation between otherwise valid complete captures restarts the consecutive series. Three full-FD9/SHA vectors must agree for at least two seconds, within 30 seconds including initial capture, sync and adoption. The existing strict capture helper can itself refuse; that is an immediate failure, never a retry. At most thirty post-sync samples are attempted.
3. The exact final admitted vector is moved into `Seal(path, entries)` and fenced immediately. There is no replacement capture-baseline, chmod, fsync or repair after adoption. This first Seal remains the strict source of the **unchanged** complete cold copy and its existing copy-admission logic. Its scope ends before the explicitly authorized RW advance; it is never resealed or reused after that advance.
4. After the one proposal is signed and every RW handle, signer and TestKey/provider has been destroyed, the fresh producer state is admitted a second and final time, label `post-sign`. The old cold-copy Seal is fenced before and after its initial capture, sync sequence, every observation and final Seal construction. An action failure still attempts that peer fence and preserves the first exception.
5. Both resulting old/current Seals, the independent floor commitment, both RO passes and all final checks remain strict full FD9/SHA checks including ctime. No old/external authority, copied Seal, Core oracle or helper is refreshed. Only these two explicit fresh production states use the new admission.

Counts stay exactly seven Core authentications, four positive authentications, three exact `std::invalid_argument("Invalid beacon signing journal")` refusals, eight RO database opens/two passes, one signing-provider call, one public BLS signature, and two producer signer constructions. Actual tokens remain the records read from the stores; floors remain independently committed outside both store trees. Store/RSS/wall budgets stay 32 MiB, 4096 entries, 128 MiB and 180 seconds. Existing cold-copy counters/admission are separate and unchanged.

## Closed PASS schema for controllers

Stdout has exactly **one JSON object** and stderr is empty on success. No additional top-level keys are allowed. Booleans must be JSON booleans, integers must be JSON integers (not booleans, floats or numeric strings). All former keys and values are unchanged; the sole new key is `productionAdmissions`.

| Keys | Exact value or validation |
| --- | --- |
| `scenario`, `result` | `beacon-journal-real-physical-rollback`, `PASS` |
| `physicalPairs`, `databases`, `independentReadonlyPasses`, `readonlyDatabaseOpens` | 2, 4, 2, 8 |
| `CoreAuthentications`, `positiveAuthentications`, `expectedCoreRefusals` | 7, 4, 3 |
| `newTestOnlySignaturesCreated`, `signingProviderCalls`, `signerConstructions`, `newSignaturesInCheck`, `signersConstructedInCheck` | 1, 1, 2, 0, 0 |
| `oldPairRecentPrimaryFloorRefused`, `oldPairRecentWitnessFloorRefused`, `oldPairBothRecentFloorsRefused` | all `true` |
| `fourClosedStoresFD9ShaUnchanged`, `completeIndependentPhysicalCopy`, `externalCommitmentOutsideBothTrees`, `externalCommitmentFD9ShaUnchanged` | all `true` |
| `externalCommitmentSha256` | lowercase hex string, exactly 64 characters |
| `externalCommitmentBytes` | integer 1..16384 |
| `maximumStoreBytes`, `storeBytes` | 33554432; integer 0..33554432 |
| `maximumEntries`, `storeEntries` | 4096; integer 1..4096 |
| `maximumRssKiB`, `actualPeakRssKiB` | 131072; integer 1..131072 |
| `maximumWallSeconds`, `elapsedMs` | 180; integer 0..200000 (unchanged caller bound) |
| `independentExternalRollbackProtectionQualified`, `historicalDigestAncestryVerified`, `deliveredSignatureHistoryVerified`, `automaticRecoveryUsed`, `operatorRecoveryQualified`, `privateInputsRead`, `runtimeDutiesAuthorized`, `networkTransport`, `commonFinalityQualified`, `productionQualified` | all `false` |
| `productionAdmissions` | exact array of **two** closed objects below, fixed order |

Every `productionAdmissions` object has exactly these eleven keys, no additional keys. The label is `initial` at index0 and `post-sign` at index1; do not accept an arbitrary array or alternative labels.

| Key | Type and bound |
| --- | --- |
| `label` | exact string determined by its index |
| `entries` | integer 1..4096 |
| `payloadBytes` | integer 0..33554432 |
| `samples` | integer 3..30 |
| `stableSamples` | integer 3..30 and no greater than `samples` |
| `elapsedMs` | integer 2000..29999 |
| `ctimeOnlyChanges` | integer 0..122880 (at most4096 entries ×30 samples, including initial-anchor to first sample) |
| `durabilityBeforeSamples` | exact boolean `true` |
| `creatorEightAndShaUnchanged` | exact boolean `true`; first closed-producer capture, not an OS-birth claim |
| `exactLastCaptureAdopted` | exact boolean `true` |
| `postAdoptionStrictFence` | exact boolean `true` |

The failure object keeps its previous fields and adds `productionAdmissionLabel`, the current `productionAdmissionSamples`, `productionAdmissionStableSamples`, `productionAdmissionElapsedMs`, `productionPreAdmissionCtimeChanges`, and `completedProductionAdmissions`. Failure diagnostics do not establish successful admission or qualification. Copy-admission failure counters retain their previous meanings.

## Limits and qualification remaining

No compilation, execution, DB inspection, signature or operational action was performed by the author. Root must separately qualify this TU and its strict parser against retained Core; a source review is not a runtime result. This does not qualify an independently protected external rollback authority, digest ancestry, signature-delivery history, operator restoration, runtime duties, common-finality progression, production readiness, or Base–Nano/Doppler. Native successor04 remains FAILED. No image/Core rebuild or network campaign is part of this source proposal.
