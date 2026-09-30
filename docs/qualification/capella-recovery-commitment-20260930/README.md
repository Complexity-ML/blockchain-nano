# External Capella recovery commitment — 30 September 2026

`capella.recoveryCommitment` pins the witness intent's exact `next` sequence
and digest. A coherent older primary/witness pair cannot authorize startup
against this separately retained commitment. Check occurs under the reactor
lock before snapshot recovery, missing publication, clock or signer setup.
No commitment retains the existing witness trust model. Exact pins must be
updated by an independent operator policy; they are not a lower sequence bound.

Native qualification passes: configured runtime regression (19.37 s), nine
external commitment checks in automatic recovery (23.82 s), and interrupted
publication with correct/incorrect commitment (5.21 s). Initial native gate
and final modified gate are both retained. No source changes followed the
final qualification; manifest identifies source and binaries by SHA-256.

Runtime checks: malformed object, missing witness field, zero sequence, wrong
digest width, absent witness directory (not created), incorrect digest rejected
before clock, coherent stale pair rejected without primary token change,
stale pair accepted with its own exact pin, and current pair accepted with
current pin. The actual stale copies are taken while stores are closed.

The interrupted-publication fixture covers retained WAL suffix, removed primary
suffix and witness-before-primary interruption. Correct pin completes recovery;
incorrect pin leaves the primary checkpoint token unchanged. Native subprocess
fault coverage is controlled WAL interruption, not physical power-loss proof.

Open: Linux ARM64/x86-64 qualification, restored signer/journal protection,
common finality after whole-node restore, externally protected commitment
storage, offsite backups, unexplained WAN attempt A and global production gates.

## Initial Linux attempt and fixture correction

Core commit `172b245` passed the two runtime gates on ARM64 (9.09 / 10.15 s).
VPS configured runtime passed (33.90 s), automatic recovery failed (35.37 s):
`RocksDB: IO error: While opendir: .../key-62: Too many open files`.
Container exit 8, no OOM; logs/state are preserved under `linux-initial`.
The VPS image's unchanged default soft/hard nofile limits are 1024 / 524288.

The stale restore fixture kept its runtime and Blockchain alive while opening
current signers. The test now ends that fixture's scope first. Production code
and descriptor limits are unchanged. Corrected native automatic gate passes;
Linux validation of the corrected fixture remains pending. The configured
native gate was also bound to the final initial harness (19.04 s); its earlier
19.37 s output is retained rather than treated as the final test binary.

## Corrected fixture under default descriptor budget

Corrected automatic recovery passes ARM64 (10.76 s), but VPS still fails
(35.75 s) opening `key-63`: too many open files. The improved scope lifetime
alone is insufficient under soft nofile=1024. Both results are preserved under
`fixture-fix`. No successful VPS qualification is claimed for this attempt.

Deployment Compose, smoke and multi-host launchers now explicitly set a
configurable nofile budget (65536 by default). This is an OS resource policy,
not a validator cap or consensus change. Qualification with that budget is
pending; runtime code and corrected fixture remain unchanged.

## Qualified deployment descriptor budget

On deployment commit `e7a4c1f`, C++ sources/test are unchanged from fixture
commit `cfbf057`; core remains `172b245`. The existing binaries are reused,
with hashes and build revision printed before testing. Under explicit
nofile=65536:65536, configured/automatic runtime gates pass ARM64 (8.88 / 10.33 s)
and VPS (32.88 / 35.54 s). Containers exit 0 without OOM. Nine external
commitment checks pass on both architectures. Corrected native automatic
recovery passes in 23.91 s. The same VPS binaries failed under soft 1024 and
passed under 65536; no further source change or recompilation occurred.

Compose JSON resolves the same descriptor budget; shell and Node launchers
pass syntax checks. Full multi-host/flood/network tests were not rerun for
this operational policy change. Interrupted WAL publication with commitment
remains native-only; signing from a whole restored backup with common finality,
independent commitment retention and full production closure remain open.
