# Automatic witnessed Capella recovery — 27 September 2026

Candidate: explicit `automaticRecovery: true` in the Capella configuration,
with `recoveryWitness`, a retained `initial` snapshot and `restart: null`.
The option defaults to false and does not change existing startup policy.

Bootstrap is allowed only if neither store contains a checkpoint. If either
store contains one, the existing witness reactor must verify the publication
intent and the primary token. Missing, corrupt or inconsistent recovery data
must not fall back to the initial snapshot. Witness network binding is checked
before reading its checkpoint. A first test exposed a missing binding step;
the initial failure is retained.

The new runtime scenario keeps its initial configuration through restart,
executes signed transactions with 64 synthetic keys, checks retained identity
and checkpoint, rejects malformed policy/missing witness/null initial settings,
and tests refusal when a primary checkpoint remains but the selected witness
is empty. The existing explicit-recovery scenario is the regression gate.

Qualification status: both native gates PASS (25.17s explicit policy, 31.13s
automatic policy). No Docker/VPS deployment.
The whole-container SIGKILL campaign with unchanged configuration remains to
be run; earlier SIGKILL evidence used an explicit configuration transition.

Both stores lost or rolled back together cannot be detected by this option.
The witness must remain a trusted persistence boundary; this feature does not
provide independent off-host rollback protection. No EIP or production closure.
