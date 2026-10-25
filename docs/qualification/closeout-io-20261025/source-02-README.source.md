# Closeout I/O checkpoints and prospective serial inventory

SOURCE ONLY. Based on Root-declared repository `2ec642be`; no program, test,
Docker, network, campaign, seed, key or private artifact was used by the author.
The repository and all prior generations/qualifications remain unchanged.
Generation01 remains beside this package with `NO-GO.source.md`: its new
interruption guard refused closeout I/O. It was rejected before execution.

## Delivered behavior

`run.mjs` defines one `checkCampaignIo()` before the first spec read. It uses
the existing interruption and optional campaign budget; it creates no clock,
deadline, profile or authorization. `observe`, `save` and `fence` call it before
and after asynchronous I/O and around buffer processing/admission. A late
operation rejects before subsequent reads, writes or NEW-map admission.

Each operation captures its entry phase/signal. A pending WORK read cannot borrow
the new signal when `enterCloseout()` aborts the old one. New bounded closeout I/O
can proceed after the first work interruption, which stays retained for the
FAILED terminal verdict. A signal received during closeout is retained separately
and refuses subsequent I/O checkpoints. The original overallEnd is never moved.
Existing signal handling still cancels commands during WORK; this patch does not
add a new closeout command-cancellation policy.

Every successfully opened observation/write/directory descriptor enters an
unconditional `finally` close. In particular, the initial observation `fstat`
now runs inside `try`, closing the descriptor if that stat rejects. Close itself
is always attempted even after interruption. A close failure cannot erase an
existing I/O/interruption error: the primary is
rejected and the secondary is attached when possible. If close alone fails, it
rejects. A close refusal is not claimed as a proven closed descriptor. Observe
admits its first held row only after successful close/check and rechecks the
existing authority rather than replacing it.

The existing file type, bound, full-nine material, SHA, known-byte and NEW
first-admission predicates remain strict. The existing NEW sampler receives a
globally checked `isRetained` callback; its sample/span/ctime rules are unchanged.
Held authorities are never refreshed. Existing stop/join/owned cleanup and the
complete fresh `finally` body are unchanged.

Two exclusive reports are written before export and before the final launch
regate: `closeout-budget-before-export.json` and `closeout-budget-before-launch.json`.
They contain counts/byte totals, original resource bounds and requested command
timers, with no private locators, keys or ledger bytes. They use the original
remaining global budget. A late report refuses export/launch. The latter report
is followed by the existing remaining-work calculation, including its write cost.
`linux-owned-files.mjs` adds only a readonly count/byte getter; its I/O predicates
and command flow are unchanged.

## Explicitly open

The report is **non-admissive**: `completeRequiredReserveMs:null`,
`admissionComplete:false`, `reserveSufficiencyQualified:false`. Neither a tiny
nor a large declared reserve makes it true. No new V3 schema, arbitrary cap,
larger budget, longer duties, campaign or launch authority is introduced.

Filesystem awaits/fsync/close and OS scheduling have no physical upper bound
here; checks after an await detect lateness but cannot cancel that await. New
artifacts, Linux authorities/nested files and owned readonly jobs grow through
closeout. A partial sum of requested command timers is not a complete worst-case bound.
Root's external terminal/owned-process observations remain required. Reserve
completion, historical audit RAM/CPU and production readiness remain OPEN.
The old `captureOwner` and `lifecycleCheck` interruption predicates are unchanged;
complete terminal capture after SIGINT therefore remains explicitly OPEN. This
lot repairs the new I/O guard regression without silently allowing resumed
duties or altering those historical callers.

`SERIAL-INVENTORY.source.md` records the actual nested stages and multiplicities.

## Source qualification requested from Root

The new `closeout-io.test.mjs` has twelve targeted synthetic groups. They extract the
actual `observe/save/fence`, signal dispatcher, pre-export caller, terminal sequence and Linux getter;
they check successful exact observations, late awaits, original errors, closed
FDs, no later read/write/admission/export, readonly counters and an explicitly
incomplete budget report. No capsule, database or actor is involved.
The added cases also cover old WORK pending I/O across closeout, new bounded
closeout I/O with a FAILED verdict, a second interruption, late/refused close,
and primary plus secondary close failures.

The new guard generation preserves every historical map, pins the three runtime
changes and adds the pure report to the closure. `hashHarness` already discovers
its static import; its first test now asserts that dependency. Existing NEW and
terminal fixtures receive only the new helper dependency as a noop because they
have no real filesystem waits; all prior assertions remain unchanged. The new
twelve groups exercise the real helper with an original global clock instead.

Root can overlay these seven source files on the matching checkout and qualify
the affected groups in its existing Docker test lane. Recommended selection:
the twelve new groups, all six guard/NEW groups, the seventeen terminal groups whose
extracted fence now has that dependency, and the first harness closure group.
Other harness tests and prior consensus/EVM suites need no replay for this patch.
Those are proposed commands/scopes only; **no tests have run in this package**.

No old success/failure is requalified, no Core/CMake change or image build is
requested, and the unfinished physical reserve guarantee is not marked solved.
