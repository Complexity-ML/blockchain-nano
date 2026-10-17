# Reactor runtime: selected code index and Root admissions

SOURCE ONLY. This proposal has not been imported or executed by its author. It emits code metadata only after Root supplies externally pinned, completed producer results. It neither prepares nor authorizes campaign 03.

`emit.py` has two explicit modes. `audit-amd` is for Root on the VPS (UID/GID 1000); it reads the current AMD runtime binding and the current P2P product table, their public results and terminal fences, then checks exactly seven selected software files. `emit` is for Root on the Mac (501:20); it joins the Native and ARM selections with the externally pinned public AMD audit. No mode invokes Docker, SSH, a compiler, a qualified software executable, a database, a seed, or a signer. The Mac mode invokes only the pinned local Git metadata utility and Node's existing pure admission/locator APIs.

## External control

`CONTROL.example.json` contains TWO incomplete examples. Root writes ONE selected object into a new control file, fills every required null from the retained producer authorities, changes `ready` to true, and supplies that control's exact SHA-256 and byte count on the command line. The example itself is not executable authority. No hash or original row is inferred from a current observation to repair an old mismatch.

Every descriptor is `{path,sha256,bytes}`. `worker`, `tools.git`, and `tools.node` additionally require `material`: the retained original Python nine-field row plus `sha256`. Tool paths must resolve to the actual canonical regular file. Producer `completion` is an existing independently pinned terminal/fence artifact plus exact expected `checks`; `checks.passed` must be true. Root replaces the example checks with fields actually emitted by that producer, while retaining `passed:true`. An existing `firstFailurePreserved:true` or `oldAuthoritiesRefreshed:true` is always refused.

The Native producer must be `nano-native-reactor-runtime-relink-software-v1`. Programs join `programs` to `products[path]`; Core joins `core` to `files[path]`. It deliberately does not use the older `newCore` shape. The ARM producer joins the completed result/binding to the pinned flat `rootHeldAfterWorkerSnapshot` and checks the seven `runtime-programs` host files against those original rows. ARM02's public result, snapshot and binding pins supplied by Root are included in the example; its terminal fence still needs its full externally known pin.

For AMD, `root` is the completed runtime producer directory: five programs are under `outputs`, Core under `baseline`. Its original GNU nine-string rows are converted to exact Python integers, including nanoseconds and timezone, only on that same VPS filesystem. The P2P product table is the NEW status-qualified producer's `products.nano-p2p.hostFD9`. Native P2P uses its new `products.private.json` row directly. ARM P2P likewise uses the NEW producer's hostFD9 row. Mac-bind `guestStatSha` is never interpreted as host FD identity. Root supplies the three new P2P physical locators and original product-table pins; old network locators are not silently reused.

The public AMD result contains seven software locators/pins and the six selected producer pins, without raw private tables. Root reads it back, pins it externally, and puts those same six pins into the Mac control's `amdProducerPins`. The Mac mode has no remote connection and does not claim to re-observe the VPS at its later emission time.

## Emission and source join

Both modes refuse unless the current runtime results are terminal PASS with the exact relink-only counts: Native 7M/7 links/8 CLI, ARM 6M/6 links/7 CLI, AMD 5M/5 links/6 CLI, zero CXX/archive/P2P commands. The selected current Cores and the three status-qualified P2P software hashes are fixed in the source. Product software is always checked against its retained original full nine fields plus SHA, owner, nlink and modes, before and after selection. Metadata descriptors are SHA/bytes authenticated and retain their first FD/path observation through the final fence; this does not recover a missing historical metadata row.

The Mac repository must be clean main at the control's exact `sourceBaseRevision`. Selected `src`, `include`, `network`, `third_party`, `tests/support`, `tests/capella` trees and CMake must equal runtime commit `1925eeeabc9f7d0aef41f2c3fe43c11c31f1f352`. Network and `third_party/json.hpp` separately join P2P commit `00154f57d4b6e1515d60edeae4e0ab38c42da28a`. There is no claim that six trees or current CMake still equal historical 68d; the multihost tree is not a compiled-consumer qualification. The two pure API modules are individually content-pinned and fenced at the current clean source revision.

The historical RPC index `3ae2a037.../9598` remains immutable. The new output uses this acyclic order:

1. New 24-entry index (9 Native, 8 ARM, 7 AMD) and source join.
2. New source manifest pinning those two artifacts and producer-result pins.
3. Three admissions, exactly the existing 13-field API shape, each pinning that manifest.
4. Separate emission receipt, pure-validation result and final fence.

No admission is inserted back into the manifest it names. The private locator spec exists only to run `validateFreshOwnerAdmission` and `freshSoftwareLocators` for all 24 artifacts; the full campaign spec is not validated. Native Core may remain at its qualified D09 physical path. Linux programs map to `/outputs`, P2P to `/network-build`; AMD Core retains its actual `baseline/libnano_core.a` absolute file locator. No software copy is made.

The new control and newly written metadata may settle only BEFORE their first hold: known SHA/bytes on every observation, at least three consecutive identical full rows spanning at least two seconds, maximum 30 seconds. Changes reset only this never-held series. Held rows are immutable. Output is a fresh, exclusive `reactor-runtime-*` namespace outside R, files fsynced with their directory. Known output pins come from the intended serialized bytes, never a replacement readback. Observations are retained privately on failure when the new output exists. A first failure is preserved; no successful result is published after a failed final fence.

Root invocation, after filling and externally pinning each separate control:

```text
python3 /absolute/emit.py audit-amd /absolute/new-control.json CONTROL_SHA CONTROL_BYTES
python3 /absolute/emit.py emit /absolute/new-control.json CONTROL_SHA CONTROL_BYTES
```

The cooperative 240-second operation budget includes bounded 20-second Git and 30-second pure Node calls; each first-admission window is bounded to 30 seconds. Blocking filesystem I/O is not a hard real-time watchdog. There are no child process groups, service discovery, signals, image/volume cleanup, package installation, network access or remote transfer in this tool.

The result qualifies the selected code-admission assembly only. It does not upgrade existing runtime images, refresh old software identities, retarget old 24-asset admissions, execute tests, assert complete loader/SDK qualification, allocate validators, resume duties, produce consensus signatures, or authorize any campaign or production deployment. Root remains responsible for later current-code launch fences, exact consent mapping, source publication, and any separately authorized campaign.
