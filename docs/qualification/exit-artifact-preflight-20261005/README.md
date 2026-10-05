# Current-runtime exit harness preparation — 2026-10-05

Runtime `ab10b8ae5dd50c7edf6f919ed440aa51a46e5e0e`, before this harness commit main `0950c18e`. This lot changes only the existing private two-node Capella exit harness, its artifact selector, and operations CI coverage. No runtime compilation, image build, new network, real funds, sale or public deployment.

The old harness selected a mutable builder tag and obsolete native/build-volume paths. It now requires `NANO_TEST_EXIT_ARTIFACT_MANIFEST`: immutable AMD64 image, explicit readonly build volume/subdirectory, source revision marker and SHA256 for native/VPS nano, P2P and fixture exporter. Preflight refuses mismatches before generating the fixture. Terminal success also rechecks executables and the transitive harness inventory. CLI revision is the harness Git revision on both hosts; runtime source continuity is checked separately against the manifest revision. Optional private monotonic-clock configuration is validated against exported profile bounds. Native and VPS fixture still use 32 public synthetic keys each, test-only shortened exit/withdrawal parameters and automatic recovery; those are not public recovery guarantees.

Evidence: current native programs hashed, readonly VPS image architecture/ID, revision and three programs independently read and verified; current exporter generated the exit fixture and both clock configurations accepted the explicit 250ms budget. `operations.log` has terminal 0, **460 passed, zero failed/skipped**, including five adversarial artifact-selection tests. Syntax and diff checks passed.

**This is preparation, not a newly qualified WAN exit/recovery/withdrawal.** The existing rotation campaign remains running separately and unchanged. Wait for its terminal result before launching the next network campaign to avoid resource interference. Historical exit results and failures remain intact; current complete validator lifecycle, sanctions, production scale, independent review and financial launch requirements remain open.

Invocation after pushing/pulling the reviewed harness, using a newly chosen output directory:

```sh
NANO_TEST_EXIT_ARTIFACT_MANIFEST=/absolute/artifacts.json \
NANO_TEST_WALL_CLOCK_DRIFT_MS=250 \
node tests/capella/exit-multihost.mjs ubuntu@145.239.199.166 "$(git rev-parse HEAD)" /absolute/new-exit-output
```

The archived manifest records this machine's current artifact locations; the runner revalidates them and does not silently select replacement artifacts. No new WAN campaign was launched by this lot.
