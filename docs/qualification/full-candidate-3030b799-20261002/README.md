# Coherent full candidate ARM64 — preparation active

Source 3030b799. Ten Capella cases already terminal/pass with unchanged inputs;
P2P all-target build completed from the same source, core all-target rebuild is
still active using existing image/volume. Configured CTest registry has 422 cases;
this is a selection count, not a result. During-build commands can be absent
until their targets build. The original full-docker runner audits complete
commands, explicit timeouts and no skips after build before running CTest.

The legacy docker-manifest.cmake captures command[0] and nano-p2p, omitting
some executable arguments. A separate observer now captures nano,
nano-finality-crash-fixture and nano-altair-probe after their build completes
and while no CTest test container exists, then will compare them after terminal
CTest. Actual before-test hashes and UTC timestamp are archived here. No source
mutation, changed test, new image, relaxed limit or interrupted build.

Supplemental observer is active, no after-test result yet. Full CTest has not
started at this preparation observation. Existing target tests/registries,
old failures and optional external corpora remain distinct from production
qualification. AMD64 Capella parent still running; new candidate still queued.

## Preparation terminal, full CTest active

P2P all-target build and core all-target build succeeded. Final configured
registry: 422 cases, all commands present, explicit timeout everywhere, complete
selection. 200 primary/network executables measured plus the three supplemental
helpers. Sources before/after build, registry and main executable manifests
before CTest match. Before-test artifacts archived in preflight-arm64; P2P
source/program evidence in network-preflight-arm64. Tests are confirmed active;
no terminal suite result or after-test helper comparison yet. The observed
registry during build remains separate and cannot substitute for this final
complete registry. Other production gates remain open.

## AMD64 complete-regression controller queued

Controller is confirmed alive on the authorized VPS, waiting for the currently
active ten-case Capella candidate campaign. It requires all ten pass, zero skips,
terminal exit codes and unchanged sources/programs/libraries. Then it rebuilds
all P2P and core targets in the existing image/volume. No extra concurrent load.
Manifest uses the strengthened main tooling (explicit node and executable
arguments) mounted separately as versioned control data, not over runtime source.

Registry and executable metadata are transferred to the Mac; existing Node
inventory validates complete selection, explicit timeouts and no missing
commands. Only a successful host inventory allows full remote CTest. After
terminal results the host verifies exact case coverage and full EEST log, with
source/program/library comparisons. No Node installation on VPS, new image,
funds, public deployment or source mutation. Control scripts syntax checked;
actual full candidate preparation/execution not reached yet. This is prepared
orchestration, not a qualification result.
