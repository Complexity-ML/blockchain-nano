# Missing Linux registrations: authoritative configuration audit

Native cached configuration contains 422 names, Docker contains 400.
Thirteen omissions are fault-injection scenarios still gated by APPLE:
4 storage WAL failure/retry, 2 BFT signing WAL, snapshot, fork checkpoint,
2 execution checkpoints, Capella execution owner sync/witness, pruning sync.
Linux's test-only fsync/fdatasync injector already exists and is linked into
other qualified fixture targets. Bellatrix also has two __APPLE__-gated fault
branches inside existing scenarios. These are coverage omissions, not proofs
that the runtime is broken or that their Linux behavior is already qualified.

Nine other names require optional fixtures/tools: slashing interchange corpus,
2 Node harness tests, 2 Node sustained recovery probes, 4 Assertoor tests.
Native cache paths were inspected and identify the optional dependencies;
Docker's prebuilt builder does not include them. No optional test can silently
be counted as Linux-qualified.

The attached patch gates fault tests by actual injector target availability,
separates the macOS injector definition from portable registration, and uses a
feature macro for Bellatrix fault branches. git apply --check succeeds.
**Patch prepared, not applied or compiled**: source freeze preserves the live
400-test build. Apply after its authoritative terminal state, then compile and
exercise actual injected EIO with nonzero failure counters and compare exact
new registrations. Node/Assertoor/corpus scopes remain separate requirements.
No runtime node is linked with the injector. Production criteria stay open.
