# Canonical cold deposit context — ARM64 and AMD64 qualification

Candidate `07509b17893b16ad8d7105aa59a31d403cf41160` passes both final10 external
envelopes. This qualifies the component and tests described here, not production
readiness, a live network or a shipping release. Git publication/delivery is a
separate Root operation; the test receipts do not attest a push or VPS update.

## Final evidence and exact execution scope

| Evidence | ARM64 Docker | AMD64 Docker |
| --- | ---: | ---: |
| External final result / actual exit | PASS / 0 | PASS / 0 |
| Whole invocation | 138042 ms | 422608 ms |
| Fresh daily CTest/JUnit | 25/25; failed0/skipped0 | 25/25; failed0/skipped0 |
| Daily execution | 105449 ms | 367129 ms |
| Same-source inherited legacy JUnit | 1/1; failed0/skipped0 | 1/1; failed0/skipped0 |
| Core / ELF / independent dependency joins | 145 / 32 / 177 | 145 / 32 / 177 |
| CLI `nano --help` | Exit0 | Exit0 |
| Held FDs closed / secondary failures | 1908/1908 / 0 | 1908/1908 / 0 |
| Signed-driver RSS high-water | 25636 KiB | 27648 KiB |
| Component-driver RSS high-water | 28992 KiB | 30656 KiB |

Coverage is26 registered cases per architecture:25 newly executed in10 and one
legacy case actually executed in09, inherited on the same07509 source with its
exact JUnit/full log and current final fences. The legacy was **not replayed in10**.
Its old35-checkpoint bound, visit10 cancellation, exact `[3,35,34,35]` boundaries
and107 typed cancellations pass. This is not26 fresh executions in10 and does
not relabel the failed09 envelope.

[ARM report](proofs/arm/result-public.json), [AMD report](proofs/amd/result-public.json),
[summary](proofs/summary-public.json), [file pins](proofs/FILES.json).
The two report SHA-256 values are respectively
`cae3ffa6ecf83eb18114559c28effa0ed5218cc74ad3b426bf704e4fe0c91c07` and
`95cfef3272652a8f7c970e33cc48503fea323b7cb9ba1a506f3c94012c2dbf39`.
Complete logs, JUnit, producer admissions, executable and header provenance are
retained in the public proof pack. CLI smoke is not P2P/network qualification.

## What changed and which compilation is inherited

Initial candidate1bc added audit-local canonical ancestry, cold tree blobs,
stream/ForkStore integration, STORE serialization guards and regressions from
fb726c9a (21 files, approximately1518 additions /65 deletions). Fixture fixes
035/992 derived the public EIP-55 sender and used child base fee +1wei with full
upfront/nonce/gas/value checks. Final07509 changes one Core CPP and the legacy
test CPP, not headers or CMake. It restores hot-prefix checkpoint compatibility
without removing cold-context cooperative checks or increasing budgets.

The two CPP units were rebuilt in08. The145-member current Core combines144
unchanged current objects with that corrected Core object. Full09 completed
legacy and the remaining build:30 Core-linked ELF relinks plus one unchanged
standalone ELF from07. Full10 performs no configuration, compilation, relink or
legacy replay. It uses six pinned08 products and externally pinned09 build/
legacy evidence, then verifies current source/object/header/ELF relationships.
Old992 archive authority is not reopened at its rewritten path or refreshed.
No historical full9 continuity is invented. Library inputs retain their exact
per-architecture commitments; binary identity across architectures is not claimed.

## Real runtime observations

Both final10 logs contain seven component groups with96 execution headers,
32 real deposit-contract events and142 full proofs. They cover exact old votes/
SSZ, siblings/corruptions, declared capacity, typed cancellation, leases and
closure; source DB namespace/full9/SHA remain unchanged. Component Beacon
envelopes are synthetic; the separate signed Core case supplies genuine admission.

Signed nonzero replay:96 descendants,16 transactions/topups,97 visits/checks,
16 old full proofs, nine prunes,64 registry entries and972 fixture BLS signatures.
Selected slot63 precedes final warm anchor80. Observed97 cold tuples /243550
logical bytes,479 blobs /41919 bytes, final cache1 record /85 bytes, inline
ancestor peak0 and warm peak33. Its configured cache quota is1 record /256 bytes.
These are each final10 run's own metrics, not values borrowed from Focused02.

[Cross-architecture comparison](proofs/cross-architecture-comparison.json) checks
15 native fields exactly, without normalization, including the three roots:

- Head `0x720bfce96300fb33b4fba729304d3e07580ac02a13737d8201bcbe4af82315e9`.
- Execution block `0xa455a264ee98b67c90dc2718d02d7d558af3f7a17764f2a88fe55e0224e62a89`.
- Execution state `0x2ac73471d402d44acf5821613c528d0a971df37b2a2740107b0448d791e406c3`.

The D0 corpus also passes1026 signed blocks plus stalled255: four groups, nine
Core and five input refusals,1027 cold tuples /2848822 logical bytes, zero blobs,
inline peak0 and33 warm records. Fixed corpus generation is not rerun. Producer
finalized epochs10/126 are fixture checkpoints, not historical fork-choice or
live network-finality qualification. RSS includes each whole fixture/test driver;
logical bytes are not physical disk/WAL or global memory bounds. Per-driver
300s/128MiB guards remain cooperative, not a whole-pipeline hard wall.

## Eighteen failed envelopes stay FAILED

| Invocations | Retained cause |
| --- | --- |
| ARM01 / AMD01 (2) | Eight fixed-array `hex` overload errors. |
| ARM02 / AMD02 (2) | Missing deposit-ancestor validator declaration/include. |
| ARM03 / AMD03 (2) | Tracked `.ipp` omitted from caller archive. |
| ARM04 (1) | Compiler/probe exit0 followed by path-alias verifier false absence. |
| ARM05 (1) |24/25; lowercase sender assertion versus EIP-55. |
| ARM06 / AMD06 (2) |24/25; legacy fee cap below child base fee. |
| Focused01 (1) | Controller metadata assumption refused before compilation. |
| Focused02 (1) | Functional96/16 output passed, but four NEW artifact fences failed. Physical cause remains unqualified; later hash matches do not retroactively qualify it. |
| ARM07 / AMD07 (2) |25/25 selected tests passed; extra legacy bound35 failed. |
| ARM08 / AMD08 (2) | Legacy-build exit0, then missing `elfMachine` before tests. |
| ARM09 / AMD09 (2) | Legacy107/build passed, then wrong universal-relink assumption rejected a standalone target; no daily/CLI. |

Total18: nine ARM + seven AMD + two focused, all preserved with their failed
results. Final10 does not rewrite old outcomes, including Focused02's fences.

## Limits and separate launch specification

This is cold **audit scratch** for stopped, read-only inputs. Live persisted
pruning, primary DB migration, reopen/restart and a persistent v4 format remain
OPEN. Legacy STORE snapshot writers reject ephemeral cold state; the explicit
`History::snapshot()` legacy-v1 export remains. No scratch deletion is inferred
from a diagnostic locator. Cancellation may leave charged unreferenced blobs.
In-flight close/drain and global RSS/disk/latency guarantees remain OPEN.

Five CI source-control files are byte-identical between their host16/16
qualification and07509; that exact scope is reused without another invocation.
It is not a paid CI job. CI billing, bitwise reproducibility, shipping P2P image,
native-Mac runtime, network restoration, full validator cycle, WAN anomaly and
independent security review remain OPEN. No image was built and no network campaign was started.

Separately, the user's delegated “le mieux” choice retains a working design of
one whole Nanobees on Base for one whole native NANO, covered over the entire
preemission with bidirectional conversion. It does not define final atomic
decimals, supply, fees or sale terms, lock actual reserves or authorize funds/
sale. USDC price and USDC bridge remain distinct. The zero-outstanding gateway
cannot import preemission by a counter change. See the
[asset specification](../../NANOBEES-BASE-ASSET.md) and
[fair-launch requirements](../../../contracts/FAIR-LAUNCH.md).
Core tests do not qualify this economic route. All bridge/Doppler production
gates remain OPEN; no EIP/fork activation or production readiness follows.
