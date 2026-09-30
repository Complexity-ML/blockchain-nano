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
