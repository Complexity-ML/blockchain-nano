# Explicit V1 → V2 delta

V1 remains immutable, manifest `81f87330…` /2,200 bytes. V2 is based on the same
public Git6183 source; neither V1 nor V2 has been executed by its author.

* `run.mjs`: add a cleanJoins set, populated only after both successful clean
  join predicates. Every capture requires both entries; current completed
  captures also gate combined publication. Move original-source/artifact/index/
  known-file families after the final software check and late-probe cleanup.
  Capture the ownedRuns boundary before those families and clean only their
  newly added readonly operations, with exact ownership guards and FAILED on
  a late stop. No subsequent software replay; final absence/held/output checks
  follow. All old fence pins and source-copy predicates stay byte-identical.
* `terminal-evidence.mjs`: byte-identical to V1, SHA `c252de8c…` /1,776 bytes.
* `terminal-evidence.test.mjs`: correct held.path; add explicit closed-but-forced
  and closed-then-throw cases for each handle, forbidden journal callbacks,
  late software/readonly operation tracking, final-boundary mutation cases and
  readonly cleanup scope/ownership cases. Fourteen groups rather than eleven.
* `lifecycle-driver-guard.test.mjs`: new to the delivered package, taken from
  Git6183. Its only changes against that base are the new run.mjs pin and the
  additional terminal-evidence.mjs pin. The remaining test logic is unchanged.

`source.patch` applies the four source files to Git6183. `delta.patch` compares
the three V1 delivered sources to V2 and adds the complete fourth source, which
V1 did not deliver. The unchanged helper emits no diff in delta.patch. This
distinguishes a new package file from the two-line functional change to the
existing guard under source.patch.

Root's runtime/public source reviewers refused V1 without executing it. The
missing-path fixture, join-success gap, observation ordering and guard pin are
known source defects, not failed executions. No old result is repaired or
relabelled by V2. Root review, parser and scoped tests remain pending.
