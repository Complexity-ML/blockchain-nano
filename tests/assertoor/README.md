# Local native Assertoor qualification

Prerequisites: built `nano`, `nano-bellatrix-transition-tests`, `nano-p2p`, Node in
PATH, and the official Assertoor binary (pinned provenance in
`docs/qualification/assertoor-native-20260926/README.md`).

Configure CMake with `NANO_NETWORK_EXECUTABLE` and `NANO_ASSERTOOR_EXECUTABLE`.
These tools remain external test dependencies; they are not linked into Nano.

- `consensus_bellatrix_assertoor`: two native cores, one validator and one
  observer. Assertoor checks both EL/CL interfaces, progress and new block bodies.
- `consensus_bellatrix_assertoor_finality`: two validator cores with 32 synthetic
  keys each. Adds `check_consensus_finality` (minimum epoch 2), then requires both
  nodes' REST checkpoints to agree and match their finalized execution blocks.
  The native harness subsequently restarts one validator and verifies continued
  propagation and common finality.

The profile is Bellatrix/Paris, with the fixture's six-second slots and eight-slot
epochs. No Ethereum default profile, production keys, assets or VPS is involved.
All nodes run on the same physical host; this is not geographic fault testing.

```sh
ctest --test-dir build-cmake -R '^consensus_bellatrix_assertoor$' --output-on-failure
ctest --test-dir build-cmake -R '^consensus_bellatrix_assertoor_finality$' --output-on-failure
```

`probe.mjs` is launched by the C++ fixture, which passes its two actual loopback
endpoints. The CLI binary is selected by `NANO_TEST_ASSERTOOR_BINARY`, with a
repository-local default. The CTest finality target selects `finality.json`
through `NANO_TEST_ASSERTOOR_FINALITY=1`; the default profile is `smoke.json`.
The probe validates each task's actual completion/result, keeps JSON reports and
logs in an announced temporary directory, and stops Assertoor in `finally`.
Deadlines: 100 seconds for smoke, 280 seconds for finality; CTest also bounds the
complete native fixture. A timeout or missing/skipped task is a failure.

Assertoor's local control API has authentication disabled for this isolated test.
Its upstream metrics listener uses a temporary port. Never reuse this setup for
public supervision without a separate access policy.

A passing profile does not cover Assertoor's full catalog, partitions, Byzantine
validators, physical I/O failures, long-running resource use, or DeFi invariants.
