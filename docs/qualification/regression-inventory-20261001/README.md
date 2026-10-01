# Complete configured C++ regression inventory — 2026-10-01

Source baseline `916c537`; inventory tool added locally after that revision.
`ctest --test-dir build-release --show-only=json-v1` collected 422 tests and
196 distinct command executables. No runtime C++ test was executed.
The configured registry includes optional local network registrations; it is
not the whole EEST fixture corpus, WAN campaign matrix or every possible build
configuration. Installed binary presence does not prove binary freshness.

Compared by exact test names against the nine-test qualified CI selection:
413 configured tests are outside that core selection. The separate transport
job covers its own 21-test registry; these numbers must not be added blindly.
All 196 local executable paths exist. No skip property is present.
15 scenarios lack an explicit CTest timeout. The sum of the existing explicit
timeouts is 68,215 seconds: a ceiling sum, not a measured duration or ETA.
The JSON lists every test, command, label, existing timeout and uncovered name.

Four new inventory protection tests pass (no failures/skips). Empty/duplicate
registries, foreign/duplicate selections, all three skip mechanisms and invalid
budgets are rejected. `--require-complete` with the actual nine-test selection
returns exit 1, as required: partial selection cannot close the complete gate.
The report always declares `inventory-only` and `runtimeTestsExecuted: 0`.

Reproduce the comparison (collect into a writable build directory):

```sh
ctest --test-dir build-release --show-only=json-v1 > /tmp/nano-registry.json
node scripts/ci/regression-inventory.mjs /tmp/nano-registry.json docs/qualification/ci-20261001/automatic-core-success/core/selection.json
```

Next complete-regression work: bind a full selection to the candidate build,
provide reviewed explicit budgets for the 15 unbounded scenarios, build and run
all required commands, then inspect actual terminal results. Historical scoped
proofs are retained and are not converted to fresh full-regression passes.
The live GitHub run `36894479093` was not interrupted or superseded.
Production criteria, independent review and launch/bridge gates remain open.

Registry and report JSON are archived as deterministic `.json.gz` files.
