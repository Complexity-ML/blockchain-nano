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
