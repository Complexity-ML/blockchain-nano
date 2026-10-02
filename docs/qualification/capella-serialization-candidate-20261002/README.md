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
