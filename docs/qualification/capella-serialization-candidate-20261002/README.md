# Capella checkpoint serialization candidate — qualification in progress

Candidate `3030b799`, parent `cac8d719`, published on
`qualification/checkpoint-serialization-20261002`; runtime not promoted to main.

Capella now embeds the validated canonical store without parsing/dumping it a
second time; moves completed output arrays and compares cache rows without
copying them on every hit. Every serialized field and checkpoint metadata remains
compared. Capella envelope overflow remains `std::length_error`. Publication,
authenticated restore, pruning validation, durable writes and witness guards are
unchanged. No limit, timeout or assertion relaxed.

Direct ARM64 test uses the actual shared helper, legacy parse/dump reference,
Bellatrix v2-v5 and Capella durable v1-v4 over store schemas v1-v3. 3,584 byte
comparisons, 3,844 rejection checks and 1,024 cancellation checks passed.
This is serialization qualification, not validator or network qualification.
Initial compile succeeded but execution failed 126 because default tmpfs was
noexec. Dedicated exec tmpfs retry passed, same image/source/capabilities;
initial failure retained. No image rebuilt.

Ten integrated ARM64 cases are running using the existing Linux build volume,
original CTest timeouts and Docker restrictions. Terminal results/manifests not
available yet. The VPS five-case run is independently still on parent cac8d719:
first activation case timed out at 360.04 s (slot 144, finalized epoch 16),
restart case was confirmed live at observation. This unresolved failure is
retained; the new source does not modify that active run.

Full regression, AMD64 candidate integration, WAN qualification and all remaining
production gates stay open. No production readiness claim.

## AMD64 qualification prepared

Separate clean source checkout at 3030b799; all 13,726 source-file hashes equal
the ARM64 campaign's source manifest. Controls select the same ten cases and
original timeouts. A live wait-and-run controller waits for the confirmed parent
container to end, then requires five terminal cases, zero skips, successful
post-test manifest and unchanged source/program/library inputs. Parent failures
are accepted as retained evidence, not treated as a reason to weaken tests.
Only then may the existing shared AMD64 build volume be recompiled. No new
Docker image. Source identity is not runtime equivalence. Current ARM64 five
cases passed, remaining cases still active at this preparation observation.

## ARM64 terminal qualification

10/10 pass, zero skips, CTest/container/post-test manifest exit 0; source,
program and library hashes before/after are identical. 517.32 s total. Original
timeouts preserved. Activation 90.9422 s, activation/restart 92.3815 s,
automatic registration 93.1471 s, reindexed activation 111.639 s, reindexed exit
110.093 s. Full raw outputs and manifests preserved; larger files gzip encoded
with hashes of original data in RAW-SHA256SUMS.

Terminal assertions: new validator votes three times after activation epoch 17,
through slot 161; pending and first-vote restarts succeed, conflicting vote is
rejected before signer, finality epoch 18. After a deposit reorg the local key
resolves to index 65, signs nine votes, both owners reopen at finality epoch 20.
Exit signed for old index 64 is rejected; exit at index 65 is automatically
included and both owners reopen with the exact exit, other validator unchanged.
This is bounded in-process lifecycle/storage qualification, synthetic time,
no network transport for reindexed cases. No WAN or physical-failure claim.

The guarded ARM64 full-regression controller has passed its terminal gate and
started recompiling all P2P targets from this same source before the existing
full-docker runner rebuilds the core and runs the complete configured registry.
It reuses the image/build volume. Network/core preparation and full regression
are not yet terminal. The AMD64 serializer qualification remains queued behind
its still-active parent; no cross-architecture final-root equality inferred.

## AMD64 build completed, ten-case qualification active

Preflight confirms source hashes identical to ARM64 and exactly ten matching
case names/command modes/timeouts/labels; only build working directory differs.
First queued-BLS case passes (9.08 s); activation is confirmed active on the VPS.
Before-test registry/program manifest archived in preflight-amd64. No terminal
AMD64 result or cross-architecture final-root equality acquired. Separate guarded
full-regression controller still waits for all ten cases to pass and manifests
to remain unchanged before touching the build volume again.

## First AMD64 activation case completed, whole campaign still active

Activation passes in 356.26 / 360 seconds. Only 3.74 seconds remain below the
original bound. [Actual case assertions and in-progress capture](activation-amd64-in-progress/README.md). Terminal ten-case and after-test manifest checks remain pending; no production or WAN closure.
