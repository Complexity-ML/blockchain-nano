# Restore runner offline finality binding

The live restore controller now stops and validates the restored writer before
issuing the native readonly audit. It prepares requiredFinalized from the last
independently observed common checkpoint, and expectedCheckpoint from the separately
captured stopped receipt; trusted profile/anchor remain separate. The audit report
must match the exact observed checkpoint, stop token/digest and terminal finality,
with both canonical replay and external finalized ancestry explicitly verified.
Older reports fail closed. Successful backup-restore is recorded only afterward.

14 new deterministic checker cases pass, zero skips. Node syntax check passes.
Native candidate 40132425 (parent 736dab46) exposes requiredFinalized in its
report. Only the changed translation unit was rebuilt directly on ARM64 against
frozen 3030b799 libraries; one actual 99-block/97-transaction positive replay
checks the new field. The unchanged helper and earlier negative cases are not
rerun. Fixture/driver hashes are unchanged. Node verifies this actual native
report against its supplied request and terminal checkpoint. Fixture-selected
pins are synthetic integration data, not independent WAN evidence.

Main runtime is still older and cannot process the new seventh request field;
this hook intentionally fails closed until the native candidate is promoted.
No live WAN run, AMD64 build, native full regression or production claim here.
Original 3030b799 ARM64/full and AMD64/Capella campaigns remain untouched.
