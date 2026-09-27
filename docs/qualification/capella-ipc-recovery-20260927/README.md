# Capella private IPC recovery — 2026-09-27

Base revision `d93b99c` plus accompanying changes.

The Capella admission service exposes its immutable wire limits and worker job
capacity. `nano_networkStatus` identifies the Capella runtime; authenticated
`nano_capellaStatus` includes execution binding, capacities, session and an
explicitly disabled network admission flag. This does not activate libp2p.

The configured runtime test invokes the existing AltairCoreRecovery state machine
against actual Rpc::handle Capella methods through a method-prefix adapter. A
previous-session job is submitted at sequence 3. Recovery must rotate the session,
retain highWater 3 and cancel/drain/forget outstanding receipts. The ledger must
be empty and networkAdmissionEnabled must remain false. Either completion or
cancellation of the racing job is valid; recovery must not leave it outstanding.

Command: `ctest --test-dir build-release -R '^consensus_capella_configured_runtime$' --output-on-failure`

Passed in 21.57 seconds. The surrounding runtime scenario still covers sixteen
blocks, sixty-four synthetic keys, witnessed restart and orderly worker shutdown.
The JSON summary retains its earlier receipt-check count; the additional IPC
assertion is in the test source and covered by its successful exit.

Scope: in-process private RPC and shared recovery state machine. This is not a
socket transport test, daemon restart test or distributed finality qualification.
Daemon profile selection, Capella protocol registration, archive/catalog relay,
peer fetch/retry and managed node startup remain open. No public launch or funds.
