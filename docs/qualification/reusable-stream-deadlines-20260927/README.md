# Capella stream deadlines and post-pruning crash recovery

**Result:** two stale-deadline races and a pruned-anchor relay defect are fixed.
Eleven new unit scenarios pass; the witnessed Docker crash/restart reaches common
finality epoch 7. Earlier failures remain archived. Global production remains open.

## Reproduced defects

Two independent negative-control runs on production source 7040d82 fail:

1. The new JSON stream test reports `already queued timeout reset completed
   reusable stream`. Its earlier timer completes I/O after both timers are ready;
   cancelling the request timer cannot revoke its already queued successful
   callback. The old timer resets a stream even after successful completion.
2. The new sender test reports `old queued idle deadline reset newly reused
   stream`. A synchronous second exchange finishes before an old queued idle
   callback runs. Stream identity and absence of pending work alone cannot
   distinguish that callback from the new idle generation.

Original failing build/CTest logs and the zero-context test patch are retained.
The first JSON test stops on parsed-read failure; raw-read/write variants are
covered by the corrected run, not claimed as separately observed initial failures.

## Correction and new coverage

JSON read/write deadlines now require the operation to remain active. Sender idle
callbacks additionally require the current unique idle-generation token. No
expiration duration, budget, message bound or consensus rule changes.

Five new cases: three queued JSON deadline variants (parsed/raw read and write),
one stale sender-idle callback, and one nine-peer ingress scenario. The latter
uses distinct synthetic authenticated PeerIds: eight idle bulk sessions fill the
existing global quota, a ninth is refused, control capacity remains available,
and releasing one idle session admits the other peer. A cancelled worker keeps
its permit until completion and cannot acknowledge afterwards. This proves
admission accounting, not fairness or consensus for a nine-node network.

Four targeted suites pass; the stream suite now contains 40 cases. Native P2P
builds. Test peers are in-memory Stream adapters, not real libp2p connections.

## Whole-node crash campaign — scenario

The Docker harness adds `crash-restart`: reach common finality with two nodes,
SIGKILL one entire container, keep it offline 96 seconds while the remaining
half-stake node advances without finalizing beyond its pre-crash bound, then
restart the same container/volume. Require preserved P2P identity, retained
pre-crash finalized header, non-regressing finality, and renewed common finality
at or beyond the surviving head's epoch. Readiness is checked separately from
finality. This is a process/container crash, not physical power-loss injection.

No VPS change or real funds. Production goal remains open. The accompanying
source patch applies with `git apply --unidiff-zero`; hashes identify the exact
candidate and binaries. Campaign results and limits are recorded below.


## Initial campaign refused an unconfigured recovery

`campaign-no-recovery-config/` retains the first failed Docker run. It reached
common epoch 2, killed node 1, kept it stopped for 96 seconds, then the node
refused startup: `Existing Capella checkpoint requires explicit restart token`.
The fixture still selected the initial snapshot and had no recovery witness.
This is an expected recovery-policy guard, not evidence of database corruption.
No protection was disabled. Its containers and network were removed and their
absence was independently verified.

The next run configures `recoveryWitness` before first boot, so the existing
witness protocol records publications throughout the run. After SIGKILL, the
harness explicitly clears `initial` and uses that existing witness for recovery.
This changes the test configuration, not C++ recovery policy. It does not qualify
unattended restart with unchanged bootstrap configuration. The bootstrap config
is saved separately before the transition; both databases share one Docker
volume, so independent/off-host rollback protection is outside this test.

The final native idle-deadline test adds an executor barrier after the already
queued old timeout; this avoids confusing a legitimately expired new idle timer
with the stale callback on slow hosts. The adjusted test passes. This test-only
edit and the recovery-harness adjustment happened after the first image context
capture. The final image was rebuilt after the anchor correction and includes
the complete current candidate.


## Witnessed restart exposed a second, real network failure

`campaign-witness-stalled/` preserves the second run. Startup succeeds with the
witness, the same P2P identity and the pre-crash finalized epoch-2 header remain,
but common finality does not reach epoch 6 within 240 seconds. Both nodes keep
producing divergent heads and finalize only epoch 2. Repeated block/attestation
catalog failures are retained; they must not be reported as successful recovery.

Code review found that `CapellaBlockDelivery` retains the daemon-start anchor.
After pruning, the local core can report an ancestor as `known:true, anchor:true`
without an exportable signed envelope. The relay rejected that response and
could not walk through the pruned history after its peer knowledge was reset.
A new `new-anchor` test fails on the old implementation (`wrong completion`).

The fix accepts the *authenticated local core's* current anchor only as the end
of a non-empty ancestor walk, with an exact requested root and `known:true`.
It does not cache a remote acknowledgement for that anchor. Each descendant
still requires the receiver's actual admission ACK. An anchor used as the target
alone is refused, so a stale catalog cannot invent a delivered block.

Six new ancestor cases cover changed anchor, anchor with data present, unknown
anchor, mismatched root, descendant rejection, and anchor as target. All 19
ancestor cases and the catalog suite pass. This identifies and fixes a concrete
relay defect; the subsequent Docker replay below qualifies recovery for this bounded case.


## Final Docker replay PASSED

The same witnessed crash/restart scenario exits 0 with the anchor correction.
The container is killed with SIGKILL and stays stopped for at least 96 seconds;
the surviving half-stake node advances its head and remains finalized at epoch 2.
The restarted node retains its P2P identity, persistent volume and finalized
header. Both nodes then reach common finalized epoch 7, beyond the required
minimum epoch 6. No conflicting finalized checkpoint is observed.

Final root: `0xda9ffeb43b3a474d27b691b3854a2b04f3d0ac0078920f52e72dd7473095c775`.

The observed offline interval is 96.801s. From the explicit recovery-configuration
transition to readiness/header verification: 2.048s. From that readiness event to
qualified common finality (including repeated observations): 139.655s. These are one-run
wall-clock measurements; they are not a latency SLA. Full receive counters,
refusals and failed ACKs remain in `analysis.json` and the logs.

All three campaigns' containers, volumes removed by the harness, and dedicated
networks are cleaned up. Independent Docker container/network inventories are
empty for the qualification label. Source and binary hashes identify the final
candidate; original failures and their fixture/configuration remain separately
archived. Whitespace-only log line endings were normalized for repository checks.

This qualifies one two-node, 64-synthetic-validator local Docker scenario with
explicit witnessed-recovery configuration. It does not qualify unchanged-config
unattended bootstrap/restart, physical power loss, adversarial storage rollback,
heavy DeFi writes, geographic WAN, many-peer fairness, long outages, or global
production readiness. No EIP completion or public deployment is claimed.
