# Fresh initial runtime clock checks

This source-only successor changes the existing fresh coordinator. It adds no launcher, profile, approval, binary, build, image or network. The source author has executed no project, Node, C++, Docker, Perl, Git or SSH program.

The actual local component already demonstrated one native export, 65 native imported pairs, 65 Linux imported pairs, Mac Docker pairs 16–31 copied and 32 cold pair reopens. That scoped result does not demonstrate the full coordinator, duties, common finality or the validator lifecycle.

## Source finding and correction of a preliminary finding

The old coordinator checked `now - genesisTime <= approved maximumDowntimeMs` once after bootstrap transport, before starting the VPS. It then waited for the remote peer, SSH tunnel, Mac configuration generation and possibly a Mac peer before starting the other owners. It neither repeated that necessary age check at these initial spawns nor compared the current durable age with the approved number of C++ catch-up steps. A preparation pass could therefore be stale when a later writer started. The C++ clock remained strict and would refuse; the missing early check did not grant valid signing authority.

A preliminary P2P concern was withdrawn after the complete locator module was read: `freshSoftwareLocators` already rejects every non-legacy P2P location. A valid current spec cannot reach the proposed explicit-P2P shadow case. This package changes no P2P path, locator rule, owner assignment or `buildFreshNodePlan` behavior.

The existing model retains exact assignments 0–15 Native, 16–31 Mac Docker and 32–64 VPS, with index 64 pending. Imported primary/witness tokens and the presigned slot-one history remain required. C++ opens all authenticated signing journals before catch-up and starts duty workers only after clock catch-up and the first clock poll. The proposal does not replace these safeguards with JavaScript claims.

Other traced joins remain unchanged (line references here identify the source baseline): `prepare.mjs:78–105` reconstructs all 65 identities from both exporter configs/environments, then replaces the cloned node's validator array with the assigned keys. `run.mjs:578–580` selects the separately admitted Mac importer/auditor and delegates to the shared Linux bootstrap. `prepare.mjs:189–205` copies the Native journals into new physical roots; `run.mjs:1111` requires actual destination audits before runtime. These source paths do not make the as-yet-unrun full preparation physically qualified.

The existing RPC and gossip adapters have matching source shapes: `src/node.cpp:77–82` creates the supervisor token and starts the selected P2P child while excluding validator secrets; `network/main.cpp:76–101` recovers the Capella IPC and publishes the protocol advertisement. `src/rpc.cpp:355` exposes the peer identity consumed by `run.mjs:1122`. Beacon immutable state/validator selection lives in `src/capella_beacon_api.cpp:20–55`; the existing coordinator reads these routes and cross-checks common finalized execution at `run.mjs:801–805`. No additional API or permission bypass is proposed. Source agreement, a peer ID, and a cold copy are each insufficient to establish real gossip, healthy duties or shared finality.

## Pure contract

`checkFreshInitialRuntimeClock({plan,node,profile,approvedClock,nowMs})` reads the already authenticated logical plan, actual selected initial node configuration and profile. It performs no I/O or mutation.

The quantities are canonical uint64 hex strings. G is `plan.anchor.genesisTimeMs`; A is `plan.anchor.timeMs`, derived by the existing planner from the actual initial durable clock. S is the actual node's `maximumForwardStepMs`; D/N are its `startupCatchup.maximumDowntimeMs/maximumSteps`; B is `maximumStartupElapsedMs`; slot is `profile.common[1]`.

The necessary conditions are:

- `now >= A >= G`, with `now` a uint64 bigint wall time.
- Generated D, N and B equal the three explicitly approved spec values.
- The original conservative global condition `now - G <= D` remains true.
- `now - A <= D` and `ceil((now - A) / S) <= N`.
- `ceil(S / slot) <= node.capella.limits.replaySlots`.

S, D, N, B, slot and replay capacity must be positive. The helper does not impose a new global relationship `D <= S*N`, reserve startup headroom, extend any bound, change genesis time or modify a profile. B is still a C++ steady-clock execution deadline, not an extra wall-clock allowance.

The public return explicitly says necessary conditions passed, `runtimeStartupVerified:false`, `genesisOrProfileChanged:false` and `productionQualified:false`.

`Date.now()` is observed on the Mac orchestrator. For the VPS, this is an orchestration preflight before dispatch, not a new measurement of the VPS clock or a guarantee about time after SSH/Docker dispatch. The VPS C++ clock must independently accept its actual local time. No remote projection or clock synchronization qualification is added here.

`launchFreshInitialRuntime(input, nowFn, launchFn)` samples `nowFn()` immediately before applying the pure check. A refusal cannot invoke `launchFn`. It accepts synchronous or Promise-returning effect callbacks; it does not start a process itself.

## Existing coordinator wiring

After the existing bootstrap copy audits, the unchanged global age condition still runs. The new helper checks all three initial configurations and writes a scoped preparation receipt. That receipt is not consumed as permission to start later.

The actual initial calls each perform a fresh check again:

- VPS Docker: after exact container-name absence, directly before its Docker run.
- Mac Docker: after the remote-peer/tunnel wait, bootnode update, immutable Linux configuration generation and exact container-name absence, directly before Docker run.
- Native Mac: after the optional Mac-peer wait, latest configuration/environment reads and private log creation, directly before spawn. Its error listener is attached synchronously in the spawn callback.

The initial-only wrapper keeps a one-shot owner map and refuses a repeated initial check for the same owner. It rechecks cancellation after its asynchronous configuration/profile reads. Exactly three distinct clock receipts are required once all first spawns have been attempted successfully.

The two Docker first calls explicitly pass `initial=true`. The default is false and all pre-existing restore/lifecycle calls remain unchanged. Native lifecycle resume is also unchanged. Later restores have a different durable clock and must not be subjected to the genesis-age guard.

Mac Docker uses the current immutable Linux generation's actual node bytes. Native and VPS use their prepared node bytes through the existing bounded FD/path reader, without admitting a mutable prepared node into an old authority. The existing VPS transport proves the delivered configuration archive; this package does not expand the VPS projection or transfer private data.

## Verification and open criteria

The new focused test exercises zero/equality/next-millisecond boundaries, the distinct conservative genesis-age bound, approved-config mismatches, replay capacity, malformed/overflow quantities, future durable time and fresh callback ordering after a simulated peer wait. Its effect callbacks count calls only. It opens no fixture, key or DB and launches no child or network.

The existing lifecycle guard changes only the two current source fixture hashes. The approved scope for Root is one run of the new focused test and one affected lifecycle guard regression; this proposal requests no repeat of C++ or old physical bootstrap tests.

Independent source review, actual tests, import and any real runtime remain Root's work. Original software/SDK/role authorities, private VPS/cross-owner permissions and all full lifecycle criteria remain unchanged. Current API binaries on each architecture, duties, shared finality, canonical exit/slashing inclusion, financial sanctions, withdrawals and the complete long lifecycle remain unqualified until their actual evidence is produced.
