# Restoration evidence guards — 2026-10-02

Baseline bec0048b direct helper reproduction accepts missing/fractional terminal
attestation counters and a conflicting same-epoch final root. These are checker
gaps, not proof of a runtime consensus exploit. The enclosing live runner already
uses stoppedReceipt to reject malformed counters; the direct helper now also
checks them independently.

The stability observer now retains the exact last Beacon checkpoint and execution
hash verified on every observer. The restore verifier requires that checkpoint,
its epoch to match the last observed epoch, positive safe integer duty counters,
and equal roots when the terminal receipt is at that same epoch. Malformed
evidence fails closed. The original three consecutive advances and deadlines
are unchanged. A later terminal epoch does not by itself prove ancestry; this
change does not close offline history/WAN or production requirements.

Four focused Node suites pass: 60/60, zero skips. Sixteen new cases cover
malformed counters, missing/malformed epoch/root evidence, conflicting roots,
accepted complete evidence and observer checkpoint/execution binding. The 44
existing cases cover adjacent stability/recovery/independent restart pins.
These are deterministic checker tests, not a new physical network campaign.

Active 3030b799 Docker qualifications and their frozen sources are untouched.
Historical captures remain unchanged; old summaries without lastCheckpoint
are not retroactively treated as passing the strengthened checker.
