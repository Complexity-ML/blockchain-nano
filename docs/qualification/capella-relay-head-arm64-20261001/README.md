# Coherent private Capella head reference — 2026-10-01

Parent source revision 395c82b; exact modified inputs identified by hashes/patch.
Private `nano_capellaStatus` now includes unsigned headSlot and headRoot from one
authenticated immutable durable publication. The daemon already tries to consume
headSlot; this field previously was absent for Capella. Root/slot are a local hint,
never authorization for a remotely supplied block, vote or checkpoint.

Mac native and Docker ARM64 builds/tests exit 0: --beacon-reads and --signer-health.
New assertions compare head hints with authenticated heads, reject cancellation
without changing checkpoint, read the same old hint while admission is held and
cancelled, and verify both fields over authenticated HTTP after signer failure.
Existing wrong-token/Origin/privacy and signer-journal regression scopes preserved.
The tests do not exercise priority delivery or distributed P2P under this change.

Existing ARM64 dependency image and build volume reused, no new runtime image.
Only diagnostic core/test executables rebuilt; volume revision marker intentionally
not advanced pending full portable qualification. Existing packaged runtime images
still identify 9513ee9; native runtime binary now differs and cannot silently be
reused with that old provenance. AMD64, head-priority relay integration and WAN
benefit remain open. All historical failures/production criteria preserved.

Initial native build compiled nano but requested nonexistent nano-capella-tests;
failed-target log preserved. Correct nano-capella-transition-tests then built and
both selected qualification scenarios passed. No failed outcome discarded.
