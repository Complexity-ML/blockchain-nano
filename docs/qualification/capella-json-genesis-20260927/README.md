# Beacon Shanghai JSON genesis — 2026-09-27

Base revision `ae056e1` plus accompanying changes.

Genesis::from_json now accepts explicit Beacon/Shanghai as well as Beacon/Paris.
Capella uses Nano/Beacon/Capella/v1 genesis domain separation and an empty
execution withdrawals trie commitment. Explicit nonempty withdrawals or a
conflicting withdrawalsRoot are rejected. Existing Paris domain behavior is
unchanged. Cancun and implicit experimental profiles remain refused here.

The startup verification test now covers initial open and reopen for both Beacon
forks: canonical state/finality reads, RPC construction, restore, legacy import,
legacy validator/template and audit cannot bypass checkpoint verification.
Capella legacy-BFT parameters and malformed withdrawal commitments are rejected.
The final gate passed in 0.57 s; chain regression passed in 3.71 s (4.28 s total).

This closes a JSON loader prerequisite for CLI startup, not CLI runtime dispatch.
run_node still expects Bellatrix and must next select the matching runtime;
Capella P2P remains unwired. Programmatically constructed historical test genesis
objects are not retroactively rewritten; their evidence remains scoped to those
objects. No database conversion or live fork migration is performed. No public
network or production activation.
